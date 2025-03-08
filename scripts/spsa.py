import argparse
import datetime
import json
import math
import random
import re
import subprocess
import sys
import threading

PARAM_INFO = {
    "futilityMarginGain": {"range": (100, 200), "default": 165, "c_end": 20, "r_end": 0.0020},
    "reductionA": {"range": (1000, 2000), "default": 1642, "c_end": 100, "r_end": 0.0020},
    "reductionB": {"range": (500, 1500), "default": 1024, "c_end": 100, "r_end": 0.0020},
    "reductionC": {"range": (500, 1500), "default": 916, "c_end": 100, "r_end": 0.0020},
    "statBonusA": {"range": (1, 30), "default": 12, "c_end": 3, "r_end": 0.0020},
    "statBonusB": {"range": (100, 500), "default": 282, "c_end": 40, "r_end": 0.0020},
    "statBonusC": {"range": (100, 500), "default": 349, "c_end": 40, "r_end": 0.0020},
    "statBonusD": {"range": (1000, 2000), "default": 1594, "c_end": 100, "r_end": 0.0020},
    "reductionInit": {"range": (1000, 3000), "default": 2026, "c_end": 200, "r_end": 0.0020},
    "counterMoveHistoryThreshold": {
        "range": (-150, 0),
        "default": -1,
        "c_end": 15,
        "r_end": 0.0020,
    },
    "aspirationDeltaA": {"range": (0, 30), "default": 10, "c_end": 3, "r_end": 0.0020},
    "aspirationDeltaB": {
        "range": (10000, 20000),
        "default": 15620,
        "c_end": 1000,
        "r_end": 0.0020,
    },
    "aspirationDeltaC": {"range": (2, 7), "default": 4, "c_end": 0.5, "r_end": 0.0020},
    "aspirationDeltaD": {"range": (1, 10), "default": 2, "c_end": 0.5, "r_end": 0.0020},
    "bonusInitialGain": {"range": (-100, 0), "default": -19, "c_end": 10, "r_end": 0.0020},
    "bonusInitialThreshold": {
        "range": (1000, 3000),
        "default": 1914,
        "c_end": 200,
        "r_end": 0.0020,
    },
    "improvementDefault": {"range": (0, 400), "default": 168, "c_end": 40, "r_end": 0.0020},
    "mateBetaDelta": {"range": (50, 250), "default": 137, "c_end": 20, "r_end": 0.0020},
    "mateDepthThreshold": {"range": (1, 10), "default": 5, "c_end": 0.5, "r_end": 0.0020},
    "mateExtraBonus": {"range": (10, 100), "default": 62, "c_end": 9, "r_end": 0.0020},
    "futilityBaseDelta": {"range": (50, 200), "default": 153, "c_end": 15, "r_end": 0.0020},
    "razoringA": {"range": (-500, 0), "default": -369, "c_end": 50, "r_end": 0.0020},
    "razoringB": {"range": (-500, 0), "default": -254, "c_end": 50, "r_end": 0.0020},
    "futilityA": {"range": (100, 500), "default": 303, "c_end": 40, "r_end": 0.0020},
    "futilityDepth": {"range": (3, 12), "default": 8, "c_end": 0.5, "r_end": 0.0020},
    "nullMoveThreshA": {
        "range": (10000, 20000),
        "default": 17139,
        "c_end": 1000,
        "r_end": 0.0020,
    },
    "nullMoveThreshB": {"range": (-100, 0), "default": -20, "c_end": 10, "r_end": 0.0020},
    "nullMoveThreshC": {"range": (1, 20), "default": 13, "c_end": 1, "r_end": 0.0020},
    "nullMoveThreshD": {"range": (100, 500), "default": 233, "c_end": 40, "r_end": 0.0020},
    "nullMoveThreshE": {"range": (1, 50), "default": 25, "c_end": 5, "r_end": 0.0020},
    "nullMoveRA": {"range": (100, 500), "default": 168, "c_end": 40, "r_end": 0.0020},
    "nullMoveRB": {"range": (1, 20), "default": 7, "c_end": 1, "r_end": 0.0020},
    "nullMoveRC": {"range": (1, 10), "default": 3, "c_end": 0.5, "r_end": 0.0020},
    "nullMoveRD": {"range": (1, 10), "default": 4, "c_end": 0.5, "r_end": 0.0020},
    "nullMoveRE": {"range": (100, 1500), "default": 861, "c_end": 140, "r_end": 0.0020},
    "nullMoveDepth": {"range": (8, 20), "default": 14, "c_end": 0.5, "r_end": 0.0020},
    "nullMovePlyA": {"range": (1, 5), "default": 3, "c_end": 0.5, "r_end": 0.0020},
    "nullMovePlyB": {"range": (1, 8), "default": 4, "c_end": 0.8, "r_end": 0.0020},
    "probCutBetaA": {"range": (100, 300), "default": 191, "c_end": 20, "r_end": 0.0020},
    "probCutBetaB": {"range": (10, 100), "default": 54, "c_end": 9, "r_end": 0.0020},
    "probCutDepthLimit": {"range": (1, 10), "default": 4, "c_end": 0.5, "r_end": 0.0020},
    "probCutDepth": {"range": (1, 10), "default": 3, "c_end": 0.5, "r_end": 0.0020},
    "ttDecreaseA": {"range": (1, 5), "default": 3, "c_end": 0.5, "r_end": 0.0020},
    "ttDecreaseB": {"range": (1, 5), "default": 2, "c_end": 0.5, "r_end": 0.0020},
    "ttDecreaseDepth": {"range": (1, 20), "default": 9, "c_end": 0.5, "r_end": 0.0020},
    "probCutBetaC": {"range": (300, 500), "default": 417, "c_end": 20, "r_end": 0.0020},
    "probCutDepthThresh": {"range": (1, 5), "default": 2, "c_end": 0.5, "r_end": 0.0020},
    "shallowPruningDepthA": {"range": (1, 15), "default": 7, "c_end": 1, "r_end": 0.0020},
    "shallowPruningA": {"range": (100, 300), "default": 180, "c_end": 20, "r_end": 0.0020},
    "shallowPruningB": {"range": (100, 300), "default": 201, "c_end": 20, "r_end": 0.0020},
    "shallowPruningC": {"range": (1, 10), "default": 6, "c_end": 0.5, "r_end": 0.0020},
    "sseThreshold": {"range": (-500, 0), "default": -222, "c_end": 50, "r_end": 0.0020},
    "shallowPruningDepthB": {"range": (1, 10), "default": 5, "c_end": 0.5, "r_end": 0.0020},
    "shallowPruningD": {
        "range": (-5000, -1000),
        "default": -3875,
        "c_end": 400,
        "r_end": 0.0020,
    },
    "shallowPruningGain": {"range": (1, 5), "default": 2, "c_end": 0.5, "r_end": 0.0020},
    "shallowPruningDepthC": {"range": (1, 20), "default": 13, "c_end": 1, "r_end": 0.0020},
    "shallowPruningE": {"range": (50, 200), "default": 106, "c_end": 15, "r_end": 0.0020},
    "shallowPruningF": {"range": (100, 200), "default": 145, "c_end": 10, "r_end": 0.0020},
    "shallowPruningG": {"range": (10, 100), "default": 52, "c_end": 9, "r_end": 0.0020},
    "shallowPruningH": {"range": (-100, 0), "default": -24, "c_end": 10, "r_end": 0.0020},
    "shallowPruningI": {"range": (-50, 0), "default": -15, "c_end": 5, "r_end": 0.0020},
    "singularExtDepthA": {"range": (1, 8), "default": 4, "c_end": 0.5, "r_end": 0.0020},
    "singularExtDepthB": {"range": (1, 5), "default": 2, "c_end": 0.5, "r_end": 0.0020},
    "singularExtDepthC": {"range": (1, 5), "default": 3, "c_end": 0.5, "r_end": 0.0020},
    "singularBetaA": {"range": (1, 5), "default": 3, "c_end": 0.5, "r_end": 0.0020},
    "singularExtentionA": {"range": (10, 50), "default": 25, "c_end": 4, "r_end": 0.0020},
    "singularExtentionB": {"range": (5, 15), "default": 9, "c_end": 0.5, "r_end": 0.0020},
    "singularExtDepthD": {"range": (5, 15), "default": 9, "c_end": 0.5, "r_end": 0.0020},
    "singularExtentionC": {"range": (50, 150), "default": 82, "c_end": 10, "r_end": 0.0020},
    "singularExtentionD": {
        "range": (1000, 10000),
        "default": 5177,
        "c_end": 900,
        "r_end": 0.0020,
    },
    "lmrDepthThreshold": {"range": (1, 5), "default": 2, "c_end": 0.5, "r_end": 0.0020},
    "lmrMoveCountThreshold": {"range": (1, 15), "default": 7, "c_end": 0.5, "r_end": 0.0020},
    "lmrDecTTPv": {"range": (0, 3), "default": 2, "c_end": 0.5, "r_end": 0.0020},
    "lmrDecMoveCount": {"range": (0, 3), "default": 1, "c_end": 0.5, "r_end": 0.0020},
    "lmrDecSingular": {"range": (0, 3), "default": 1, "c_end": 0.5, "r_end": 0.0020},
    "lmrIncCutNode": {"range": (0, 3), "default": 2, "c_end": 0.5, "r_end": 0.0020},
    "lmrIncTTCapture": {"range": (0, 3), "default": 1, "c_end": 0.5, "r_end": 0.0020},
    "lmrPvNodeA": {"range": (1, 5), "default": 1, "c_end": 0.5, "r_end": 0.0020},
    "lmrPvNodeB": {"range": (1, 20), "default": 11, "c_end": 1, "r_end": 0.0020},
    "lmrPvNodeC": {"range": (1, 5), "default": 3, "c_end": 0.5, "r_end": 0.0020},
    "lmrCutoffCntThresh": {"range": (1, 5), "default": 3, "c_end": 0.5, "r_end": 0.0020},
    "lmrIncCutoffCnt": {"range": (0, 3), "default": 1, "c_end": 0.5, "r_end": 0.0020},
    "lmrStatGain": {"range": (1, 5), "default": 2, "c_end": 0.5, "r_end": 0.0020},
    "lmrStatDelta": {"range": (1000, 10000), "default": 4433, "c_end": 900, "r_end": 0.0020},
    "lmrRDecA": {"range": (10000, 20000), "default": 13628, "c_end": 1000, "r_end": 0.0020},
    "lmrRDecB": {"range": (1000, 10000), "default": 4000, "c_end": 900, "r_end": 0.0020},
    "lmrRDecDepthA": {"range": (3, 10), "default": 7, "c_end": 0.5, "r_end": 0.0020},
    "lmrRDecDepthB": {"range": (11, 30), "default": 19, "c_end": 1, "r_end": 0.0020},
    "lmrDeepSearchA": {"range": (10, 100), "default": 64, "c_end": 9, "r_end": 0.0020},
    "lmrDeepSearchB": {"range": (1, 20), "default": 11, "c_end": 1, "r_end": 0.0020},
}

PARAM_TC_INFO = {
    "fallingEvalA": {
        "range": (10, 100),
        "default": 66,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "fallingEvalB": {
        "range": (5, 25),
        "default": 14,
        "c_end": 2,
        "r_end": 0.0020,
    },
    "fallingEvalC": {
        "range": (3, 10),
        "default": 6,
        "c_end": 1,
        "r_end": 0.0020,
    },
    "fallingEvalD": {
        "range": (100, 1000),
        "default": 617,
        "c_end": 90,
        "r_end": 0.0020,
    },
    "fallingEvalClampMin": {
        "range": (1, 99),
        "default": 51,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "fallingEvalClampMax": {
        "range": (101, 200),
        "default": 151,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "timeReductionDepth": {
        "range": (4, 12),
        "default": 8,
        "c_end": 0.5,
        "r_end": 0.0020,
    },
    "timeReductionA": {
        "range": (100, 200),
        "default": 156,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "timeReductionB": {
        "range": (30, 120),
        "default": 69,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "timeReductionC": {
        "range": (100, 180),
        "default": 140,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "timeReductionD": {
        "range": (150, 250),
        "default": 217,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "bestMoveInstabilityA": {
        "range": (100, 250),
        "default": 179,
        "c_end": 15,
        "r_end": 0.0020,
    },
    "totalTimeGain": {
        "range": (30, 100),
        "default": 75,
        "c_end": 5,
        "r_end": 0.0020,
    },
    "optExtraA": {
        "range": (50, 150),
        "default": 100,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "optExtraB": {
        "range": (50, 150),
        "default": 125,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "optExtraC": {
        "range": (50, 150),
        "default": 111,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "optConstantA": {
        "range": (100, 500),
        "default": 334,
        "c_end": 40,
        "r_end": 0.0020,
    },
    "optConstantB": {
        "range": (10, 50),
        "default": 30,
        "c_end": 4,
        "r_end": 0.0020,
    },
    "optConstantC": {
        "range": (10, 100),
        "default": 49,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "maxConstantA": {
        "range": (100, 500),
        "default": 340,
        "c_end": 40,
        "r_end": 0.0020,
    },
    "maxConstantB": {
        "range": (100, 500),
        "default": 300,
        "c_end": 40,
        "r_end": 0.0020,
    },
    "maxConstantC": {
        "range": (100, 500),
        "default": 276,
        "c_end": 40,
        "r_end": 0.0020,
    },
    "optScaleA": {
        "range": (100, 200),
        "default": 120,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "optScaleB": {
        "range": (20, 40),
        "default": 31,
        "c_end": 2,
        "r_end": 0.0020,
    },
    "optScaleC": {
        "range": (30, 60),
        "default": 44,
        "c_end": 3,
        "r_end": 0.0020,
    },
    "optScaleD": {
        "range": (10, 50),
        "default": 21,
        "c_end": 4,
        "r_end": 0.0020,
    },
    "maxScaleA": {
        "range": (50, 100),
        "default": 69,
        "c_end": 5,
        "r_end": 0.0020,
    },
    "maxScaleB": {
        "range": (100, 150),
        "default": 122,
        "c_end": 5,
        "r_end": 0.0020,
    },
    "maximumTimeA": {
        "range": (50, 150),
        "default": 84,
        "c_end": 10,
        "r_end": 0.0020,
    },
    "maximumTimeB": {
        "range": (0, 30),
        "default": 10,
        "c_end": 3,
        "r_end": 0.0020,
    },
}


def print_progress_bar(current, total, bar_size=30):
    ratio = current / total
    filled_len = int(bar_size * ratio)
    bar = "#" * filled_len + "-" * (bar_size - filled_len)
    sys.stdout.write(f"\r[{bar}] {current}/{total}")
    sys.stdout.flush()


def approximate_elo_and_error_2sigma(wins, draws, losses):
    total = wins + draws + losses
    if total == 0:
        return 0.0, 0.0
    score = wins + 0.5 * draws
    p = score / total
    eps = 1e-9
    if p <= 0.0:
        p = eps
    elif p >= 1.0:
        p = 1.0 - eps
    elo = 400.0 * math.log10(p / (1.0 - p))
    try:
        dEdp = 400.0 / (math.log(10) * p * (1.0 - p))
        sigma_p = math.sqrt(p * (1.0 - p) / total)
        sigma_elo = dEdp * sigma_p
    except ValueError:
        sigma_elo = 0.0
    return elo, 2.0 * sigma_elo


class SPSAOptimizer:
    def __init__(
        self,
        param_all,
        iterations=50000,
        A=5000,
        gamma=0.101,
        alpha=0.602,
        concurrency=32,
        save_step=1000,
        use_adam=False,
        test_rounds=256,
    ):
        """
        param_all: 結合済みパラメータ集合。各パラメータは dict で、キー:
            - range, default, c_end, r_end, origin, update
        update==True のパラメータのみ SPSA 更新の対象となる。
        """
        self.iterations = iterations
        self.A = A
        self.gamma = gamma
        self.alpha = alpha
        self.concurrency = concurrency
        self.save_step = save_step
        self.use_adam = use_adam
        self.test_rounds = test_rounds

        # 結合済みパラメータ集合
        self.param_all = param_all

        # 各パラメータの現在値 (初期値は default)
        self.theta = {}
        # SPSA 用の定数 a0, c0 (更新対象のみ)
        self.a0 = {}
        self.c0 = {}
        for name, info in self.param_all.items():
            self.theta[name] = info["default"]
            if info.get("update", True):
                self.c0[name] = info["c_end"] * (iterations**gamma)
                a_end_val = info["r_end"] * (info["c_end"] ** 2)
                self.a0[name] = a_end_val * ((A + iterations) ** alpha)

        # Adam 用の内部状態（更新対象のみ）
        self.m = {name: 0.0 for name, info in self.param_all.items() if info.get("update", True)}
        self.v = {name: 0.0 for name, info in self.param_all.items() if info.get("update", True)}
        self.adam_t = 0
        self.adam_beta1 = 0.9
        self.adam_beta2 = 0.999
        self.adam_epsilon = 1e-8
        self.adam_lr = 0.001

        self.lock = threading.Lock()
        self.global_iter = 0

        # ログファイル作成（YYYYMMDD_HHMMSS.log）
        date_str = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        self.log_filename = f"log/{date_str}.log"
        self.log_file = open(self.log_filename, "a", buffering=1, encoding="utf-8")

    def worker_thread(self, thread_id, next_save_iter):
        while True:
            with self.lock:
                if self.global_iter >= self.iterations:
                    return
                elif self.global_iter + 1 > next_save_iter:
                    return
                i = self.global_iter + 1
                self.global_iter = i

            # 各更新対象パラメータについて c(i), a(i) を計算
            c_vals = {}
            a_vals = {}
            for name, info in self.param_all.items():
                if info.get("update", True):
                    c_vals[name] = self.c0[name] / (i**self.gamma)
                    a_vals[name] = self.a0[name] / ((self.A + i) ** self.alpha)

            with self.lock:
                current_theta = self.theta.copy()

            # エンジンに渡すパラメータを作成（更新対象は±の擾乱、固定はそのまま）
            eng1_params = {}
            eng2_params = {}
            delta_dict = {}  # 更新対象のパラメータの符号を記録
            for name, info in self.param_all.items():
                if info.get("update", True):
                    base_val = current_theta[name]
                    c_val = c_vals[name]
                    delta = 1 if random.randint(0, 1) else -1
                    v_plus = base_val + c_val * delta
                    v_minus = base_val - c_val * delta
                    lo, hi = info["range"]
                    v_plus = max(min(v_plus, hi), lo)
                    v_minus = max(min(v_minus, hi), lo)
                    eng1_params[name] = v_plus
                    eng2_params[name] = v_minus
                    delta_dict[name] = delta
                else:
                    eng1_params[name] = current_theta[name]
                    eng2_params[name] = current_theta[name]

            # 2ゲーム対局を実施（score = 勝ち数 - 敗北数）
            score = self.run_cutechess(eng1_params, eng2_params)

            # SPSA勾配推定と更新（更新対象のみ）
            with self.lock:
                self.adam_t += 1
                for name, info in self.param_all.items():
                    if info.get("update", True):
                        c_val = c_vals[name]
                        d = delta_dict[name]
                        g = (score / d) / (2.0 * c_val)
                        if not self.use_adam:
                            self.theta[name] += a_vals[name] * g
                        else:
                            self.m[name] = (
                                self.adam_beta1 * self.m[name] + (1.0 - self.adam_beta1) * g
                            )
                            self.v[name] = self.adam_beta2 * self.v[name] + (
                                1.0 - self.adam_beta2
                            ) * (g * g)
                            m_hat = self.m[name] / (1.0 - (self.adam_beta1**self.adam_t))
                            v_hat = self.v[name] / (1.0 - (self.adam_beta2**self.adam_t))
                            adam_update = self.adam_lr * (
                                m_hat / (math.sqrt(v_hat) + self.adam_epsilon)
                            )
                            self.theta[name] += adam_update
                        # クリッピング
                        lo, hi = info["range"]
                        if self.theta[name] < lo:
                            self.theta[name] = lo
                        elif self.theta[name] > hi:
                            self.theta[name] = hi

                print_progress_bar(self.global_iter, self.iterations)

    def run_cutechess(self, var_eng1, var_eng2):
        """
        両エンジンに対して各パラメータを渡し、2ゲーム対局を実施する。
        (各エンジンは同じバイナリを呼び、名前とオプションで区別)
        """
        engine_bin = "./cfish_params_tc"
        cmd = [
            "cutechess-cli",
            "-engine",
            f"cmd={engine_bin}",
            "proto=uci",
            "name=plus",
        ]
        for pname, val in var_eng1.items():
            ival = int(round(val))
            cmd.append(f"option.{pname}={ival}")

        cmd += [
            "-engine",
            f"cmd={engine_bin}",
            "proto=uci",
            "name=minus",
        ]
        for pname, val in var_eng2.items():
            ival = int(round(val))
            cmd.append(f"option.{pname}={ival}")

        cmd += [
            "-each",
            "tc=10",
            "-games",
            "2",
            "-rounds",
            "1",
            "-repeat",
            "-openings",
            "file=openings.epd",
            "format=epd",
            "order=random",
            "-concurrency",
            "1",
            "-resign",
            "movecount=8",
            "score=600",
            "-draw",
            "movenumber=40",
            "movecount=8",
            "score=20",
        ]

        proc = subprocess.run(cmd, capture_output=True, text=True)
        w, l, _ = self.parse_cutechess_output(proc.stdout)
        return w - l

    def parse_cutechess_output(self, output):
        w, l, d = 0, 0, 0
        for line in output.split("\n"):
            match = re.search(r"Score of .*?: (\d+) - (\d+) - (\d+)", line)
            if match:
                w, l, d = map(int, match.groups())
        return w, l, d

    def measure_elo(self, rounds=256, concurrency=1):
        """
        全パラメータの現在値をエンジンに渡し、Nゲーム対局により Elo を測定する。
        """
        engine_bin = "./cfish_params_tc"
        combined = self.theta.copy()
        cmd = [
            "cutechess-cli",
            "-engine",
            f"cmd={engine_bin}",
            "proto=uci",
            "name=Base",
            "-engine",
            f"cmd={engine_bin}",
            "proto=uci",
            "name=Test",
        ]
        for pname, val in combined.items():
            ival = int(round(val))
            cmd.append(f"option.{pname}={ival}")
        cmd += [
            "-each",
            "tc=10",
            "-games",
            "2",
            "-rounds",
            str(rounds),
            "-repeat",
            "-openings",
            "file=openings.epd",
            "format=epd",
            "order=random",
            "-concurrency",
            f"{concurrency}",
            "-resign",
            "movecount=8",
            "score=600",
            "-draw",
            "movenumber=40",
            "movecount=8",
            "score=20",
        ]
        proc = subprocess.run(cmd, capture_output=True, text=True)
        w, l, d = self.parse_cutechess_output(proc.stdout)
        elo, elo_2s = approximate_elo_and_error_2sigma(w, d, l)
        return elo, elo_2s

    def start_optimization(self):
        next_save_iter = self.save_step
        while True:
            threads = []
            for tid in range(self.concurrency):
                t = threading.Thread(target=self.worker_thread, args=(tid, next_save_iter))
                threads.append(t)
                t.start()

            for t in threads:
                t.join()

            elo, elo_2sigma = self.measure_elo(
                rounds=self.test_rounds, concurrency=self.concurrency
            )
            self.log_file.write(f"Iteration {self.global_iter}: Elo={elo:.2f} ±{elo_2sigma:.2f}\n")
            print(f"\nIteration {self.global_iter}: Elo={elo:.2f} ±{elo_2sigma:.2f}\n")
            self.log_file.write("Parameters:\n")
            for name, val in self.theta.items():
                self.log_file.write(f"  {name}: {val:.3f}\n")

            next_save_iter += self.save_step
            if next_save_iter > self.iterations:
                break

        print("\nSPSA Optimization Finished.")
        print("Final parameters:")
        for name, val in self.theta.items():
            print(f"  {name}: {val:.3f}")
        self.log_file.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="SPSA Optimization for Engine Parameters")
    # --spsa-target により更新対象を指定
    parser.add_argument(
        "--spsa-target",
        choices=["both", "info", "tc"],
        default="info",
        help="更新対象。'both'なら全パラメータ更新、'info'ならPARAM_INFOのみ、'tc'ならPARAM_TC_INFOのみ更新",
    )
    # JSONファイルによる上書き（指定されたパラメータは固定＝SPSA更新しない）
    parser.add_argument("--json-info", type=str, help="PARAM_INFO 用 JSON ファイル")
    parser.add_argument("--json-tc", type=str, help="PARAM_TC_INFO 用 JSON ファイル")
    parser.add_argument("--iterations", type=int, default=100000, help="SPSA の反復回数")
    args = parser.parse_args()

    # --- 各パラメータに origin と update フラグを付与（初期は update=True） ---
    for key, info in PARAM_INFO.items():
        info["origin"] = "info"
        info["update"] = True
    for key, info in PARAM_TC_INFO.items():
        info["origin"] = "tc"
        info["update"] = True

    # --spsa-target により、更新対象を制御
    if args.spsa_target == "info":
        for key, info in PARAM_TC_INFO.items():
            info["update"] = False
    elif args.spsa_target == "tc":
        for key, info in PARAM_INFO.items():
            info["update"] = False
    # "both" の場合はそのまま

    # JSONファイルによる上書き (指定された場合、対象パラメータは default を上書きし、update=False とする)
    if args.json_info:
        try:
            with open(args.json_info, "r") as f:
                json_info = json.load(f)
            for key, val in json_info.items():
                if key in PARAM_INFO:
                    PARAM_INFO[key]["default"] = val
                    PARAM_INFO[key]["update"] = False
                else:
                    print(f"Warning: {key} は PARAM_INFO に存在しません")
        except Exception as e:
            print(f"PARAM_INFO JSON 読み込みエラー: {e}")
            sys.exit(1)
    if args.json_tc:
        try:
            with open(args.json_tc, "r") as f:
                json_tc = json.load(f)
            for key, val in json_tc.items():
                if key in PARAM_TC_INFO:
                    PARAM_TC_INFO[key]["default"] = val
                    PARAM_TC_INFO[key]["update"] = False
                else:
                    print(f"Warning: {key} は PARAM_TC_INFO に存在しません")
        except Exception as e:
            print(f"PARAM_TC_INFO JSON 読み込みエラー: {e}")
            sys.exit(1)

    # --- PARAM_INFO と PARAM_TC_INFO を結合 ---
    combined_params = {}
    for key, info in PARAM_INFO.items():
        combined_params[key] = info.copy()
    for key, info in PARAM_TC_INFO.items():
        if key in combined_params:
            print(f"Warning: 重複パラメータキー {key} が存在します。")
        combined_params[key] = info.copy()

    iterations = args.iterations
    optimizer = SPSAOptimizer(
        param_all=combined_params,
        iterations=iterations,
        A=(iterations * 0.1),
        gamma=0.101,
        alpha=0.602,
        concurrency=32,
        save_step=2000,
        use_adam=False,
        test_rounds=1024,
    )
    optimizer.start_optimization()
