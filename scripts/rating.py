#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
リアルタイムで1行に上書きしつつ対局スコア・Elo推定(±2σ)を表示し、
SPRT早期終了 (H0/H1) があれば新たに改行して表示するスクリプト

使用例:
  python rating.py --engine1 "./cfish" --engine2 "./cfish_target" \
      --games 512 --concurrency 32 \
      --sprt-elo0 10 --sprt-elo1 5 --sprt-alpha 0.05 --sprt-beta 0.05 \
      --engine1-json engine1.json --engine2-json engine2.json
"""

import argparse
import json
import math
import os
import re
import subprocess
import sys


def approximate_elo_and_error_2sigma(wins, draws, losses):
    """
    (wins, draws, losses) からAの勝率を算出し、Elo差とその2σ(約95%CI)を返す。
    戻り値: (elo, elo_2sigma)
    """
    total = wins + draws + losses
    if total == 0:
        return 0.0, 0.0

    score_a = wins + 0.5 * draws
    p = score_a / total

    eps = 1e-9
    if p <= 0.0:
        p = eps
    elif p >= 1.0:
        p = 1.0 - eps

    elo = 400.0 * math.log10(p / (1.0 - p))

    try:
        dElo_dp = 400.0 / (math.log(10) * p * (1.0 - p))
        sigma_p = math.sqrt(p * (1.0 - p) / total)
        sigma_elo = dElo_dp * sigma_p  # 1σ
    except ValueError:
        sigma_elo = 0.0

    elo_2sigma = 2.0 * abs(sigma_elo)
    return elo, elo_2sigma


def parse_engine_options(json_path):
    """
    JSONファイル(json_path)を読み込み、{key: value} の形で返す。
    ファイルが指定されていない、あるいは存在しない場合は空のdict。
    """
    if not json_path:
        return {}
    if not os.path.isfile(json_path):
        print(f"Warning: JSON file '{json_path}' not found. Ignoring.")
        return {}
    try:
        with open(json_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        # data は { "optionName": value, ... } と想定
        return data
    except Exception as e:
        print(f"Warning: Failed to load or parse JSON '{json_path}': {e}")
        return {}


def build_cutechess_command(args):
    """
    cutechess-cli のコマンドラインをリストで組み立てる。
    JSONファイルから読み込んだオプションがあれば、option.xxx=yyy 形式で追加する。
    """
    if args.games % 2 != 0:
        print("Error: --games must be even.")
        sys.exit(1)
    rounds = args.games // 2

    # JSONのオプション読み込み
    engine1_opts = parse_engine_options(args.engine1_json)
    engine2_opts = parse_engine_options(args.engine2_json)

    # エンジン1 (SF1) の引数を組み立て
    cmd = [
        "cutechess-cli",
        "-engine",
        f"cmd={args.engine1}",
        "proto=uci",
        "name=SF1",
    ]
    for k, v in engine1_opts.items():
        cmd.append(f"option.{k}={v}")

    # エンジン2 (SF2) の引数を組み立て
    cmd += [
        "-engine",
        f"cmd={args.engine2}",
        "proto=uci",
        "name=SF2",
    ]
    for k, v in engine2_opts.items():
        cmd.append(f"option.{k}={v}")

    # 共通設定
    cmd += [
        "-each",
        "tc=10",
        "-openings",
        "file=openings.epd",
        "format=epd",
        "order=random",
        "-games",
        "2",
        "-rounds",
        str(rounds),
        "-repeat",
        "-concurrency",
        str(args.concurrency),
        "-resign",
        "movecount=8",
        "score=600",
        "-draw",
        "movenumber=50",
        "movecount=8",
        "score=20",
    ]

    # SPRT指定
    if (
        args.sprt_elo0 is not None
        and args.sprt_elo1 is not None
        and args.sprt_alpha is not None
        and args.sprt_beta is not None
    ):
        sprt_str = (
            f"-sprt elo0={args.sprt_elo0} elo1={args.sprt_elo1} "
            f"alpha={args.sprt_alpha} beta={args.sprt_beta}"
        )
        cmd.append(sprt_str)

    return cmd


def parse_score_line(line):
    """
    "Score of SF1 vs SF2: w - l - d  [some] total"
    => (w, l, d, total) or None
    """
    m = re.search(r"Score of SF1 vs SF2:\s+(\d+)\s*-\s*(\d+)\s*-\s*(\d+)\s*\[\S+\]\s+(\d+)", line)
    if m:
        w = int(m.group(1))
        l = int(m.group(2))
        d = int(m.group(3))
        tot = int(m.group(4))
        return (w, l, d, tot)
    return None


def parse_sprt_line(line):
    """
    SPRT判定行
    例: "SPRT: llr -2.95, lbound -2.94, ubound 2.94 - H0 was accepted"
    => "H0 was accepted" or "H1 was accepted"
    なければ None
    """
    m = re.search(r"SPRT:.*(H0 was accepted|H1 was accepted)", line)
    if m:
        return m.group(1)
    return None


def print_oneline(w, l, d, tot, elo_val, elo_err_2sigma):
    """
    1行上書き表示 (改行なし)。
    Score, Elo±2σ をまとめて表示。
    """
    # 勝率/引き分け率計算
    total_games = w + l + d
    if total_games > 0:
        draw_ratio = 100.0 * d / total_games
        scA = w + 0.5 * d
        wr = 100.0 * scA / total_games
    else:
        draw_ratio = 0.0
        wr = 0.0

    msg = (
        f"Score: {w}-{l}-{d}/{tot} "
        f"(WR={wr:.1f}%, DR={draw_ratio:.1f}%) "
        f"Elo={elo_val:.1f}±{elo_err_2sigma:.1f} (2σ)"
    )

    # 上書き (キャリッジリターン)
    sys.stdout.write("\r" + msg + "     ")
    sys.stdout.flush()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine1", default="./cfish_sf15", help="エンジン1の実行ファイルパス")
    parser.add_argument("--engine2", default="./cfish_sf15", help="エンジン2の実行ファイルパス")
    parser.add_argument(
        "--engine1-json", default=None, help="エンジン1の設定オプションを含むJSONファイル"
    )
    parser.add_argument(
        "--engine2-json", default=None, help="エンジン2の設定オプションを含むJSONファイル"
    )
    parser.add_argument("--games", type=int, default=512, help="対局数(偶数が必要)")
    parser.add_argument("--concurrency", type=int, default=32, help="並列対局数")
    parser.add_argument("--sprt-elo0", type=float, default=None)
    parser.add_argument("--sprt-elo1", type=float, default=None)
    parser.add_argument("--sprt-alpha", type=float, default=None)
    parser.add_argument("--sprt-beta", type=float, default=None)
    args = parser.parse_args()

    cmd = build_cutechess_command(args)
    print("Running command:\n", " ".join(cmd), "\n")

    process = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True, bufsize=1
    )

    current_w = 0
    current_l = 0
    current_d = 0
    current_t = 0

    sprt_decision = None

    try:
        for line in process.stdout:
            line = line.rstrip("\n")
            # Score行をパース
            parsed = parse_score_line(line)
            if parsed:
                w, l, d, tot = parsed
                current_w, current_l, current_d, current_t = w, l, d, tot

                # Elo計算
                elo_val, elo_err_2sigma = approximate_elo_and_error_2sigma(w, d, l)
                # 上書き表示
                print_oneline(w, l, d, tot, elo_val, elo_err_2sigma)

            # SPRT判定チェック
            sprt_res = parse_sprt_line(line)
            if sprt_res:
                sprt_decision = sprt_res
                # SPRT早期終了を別行で表示
                sys.stdout.write("\nSPRT early stop: " + sprt_decision + "\n")
                sys.stdout.flush()

        process.wait()
    except KeyboardInterrupt:
        print("\nInterrupted by user.")
        process.terminate()

    ret = process.returncode
    print(f"\ncutechess-cli exited with code {ret}\n")

    # 最終結果 (改行)
    if current_t > 0:
        elo_val, elo_err_2sigma = approximate_elo_and_error_2sigma(current_w, current_d, current_l)
        sys.stdout.write(
            f"Final Score: {current_w}-{current_l}-{current_d}/{current_t}, "
            f"Elo={elo_val:.1f}±{elo_err_2sigma:.1f} (2σ)\n"
        )
        if sprt_decision:
            print(f"SPRT final decision: {sprt_decision}")
    else:
        print("No valid scores were parsed.")


if __name__ == "__main__":
    main()
