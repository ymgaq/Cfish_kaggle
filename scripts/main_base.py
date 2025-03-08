# import os
import subprocess
import threading
import time
from queue import Empty, Queue


class ChessEngine:
    def __init__(self, engine_path):
        self.engine = subprocess.Popen(
            [engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
            universal_newlines=True,
        )
        self.output_queue = Queue()
        self.stop_event = threading.Event()
        self.reader_thread = threading.Thread(target=self._enqueue_output, daemon=True)
        self.reader_thread.start()
        self._initialize_engine()
        self.pondering = False
        self.ponder_move = None
        self.last_send_time = None

    def __del__(self):
        self.stop_event.set()
        self.reader_thread.join()
        if self.engine.poll() is None:
            self._send_command("stop")
            self._send_command("quit")
        self.engine.terminate()
        self.engine.wait()

    def _enqueue_output(self):
        while not self.stop_event.is_set():
            line = self.engine.stdout.readline()
            if line:
                self.output_queue.put(line.strip())
            elif self.engine.poll() is not None:
                break

    def _initialize_engine(self):
        self._send_command("isready")
        while True:
            output = self._read_output()
            if output == "readyok":
                break

    def _send_command(self, command):
        self.engine.stdin.write(command + "\n")
        self.engine.stdin.flush()
        self.last_send_time = time.time()

    def _read_output(self, timeout=None):
        try:
            return self.output_queue.get(timeout=timeout)
        except Empty:
            return None

    def get_best_and_ponder_move(self):
        best_move = None
        ponder_move = None
        # info_values = {"depth": "-", "score": "-", "nodes": "-", "nps": "-", "time": "-"}

        while True:
            output = self._read_output(timeout=0.1)
            if output is None:
                continue

            # if output.startswith("info"):
            #     parts = output.split()
            #     for i in range(len(parts)):
            #         if parts[i] in info_values:
            #             info_values[parts[i]] = (
            #                 parts[i + 1] if parts[i] != "score" else parts[i + 2]
            #             )

            if output.startswith("bestmove"):
                parts = output.split()
                best_move = parts[1]
                if len(parts) > 2 and parts[2] == "ponder":
                    ponder_move = parts[3]

                # debug_info = (
                #     f"info depth={str(info_values['depth']).rjust(2)} "
                #     f"score={str(info_values['score']).rjust(4)} "
                #     f" nodes={str(info_values['nodes']).rjust(6)} "
                #     f"nps={str(info_values['nps']).rjust(7)} "
                #     f"time={str(info_values['time']).rjust(3)}"
                # )

                # return best_move, ponder_move, debug_info
                return best_move, ponder_move

    def wait_for_stop(self):
        while True:
            output = self._read_output(timeout=0.1)
            if output is None:
                continue
            if output.startswith("bestmove"):
                break

    def get_best_move(self, obs):
        t_start_best_move = time.time()

        color = obs["mark"]
        fen = obs["board"]
        last_move = obs["lastMove"]
        # move_count = obs["step"]
        time_opponent_ms = int(float(obs["opponentRemainingOverageTime"]) * 1000)
        time_self_ms = int(float(obs["remainingOverageTime"]) * 1000)

        ponderhit = False
        # sleep if < 5 ms from last send
        if self.last_send_time is not None:
            delta_t_from_last_send = int((t_start_best_move - self.last_send_time) * 1000)
            if delta_t_from_last_send < 5:
                time.sleep((5 - delta_t_from_last_send) / 1000)

        if last_move and self.pondering and self.ponder_move == last_move:
            # ponderhit
            self._send_command("ponderhit")
            self.pondering = False
            ponderhit = True
        else:
            if self.pondering:
                # stop pondering
                self._send_command("stop")
                self.wait_for_stop()
                self.pondering = False

            # set position and start thinking
            t_start_think = time.time()
            self._send_command(f"position fen {fen}")
            delta_t_ms = int((t_start_think - t_start_best_move) * 1000)
            time_self_go_ms = max(0, time_self_ms - delta_t_ms)
            wtime = time_self_go_ms if color == "white" else time_opponent_ms
            btime = time_opponent_ms if color == "white" else time_self_go_ms
            self._send_command(f"go wtime {wtime} btime {btime}")

        # best_move, ponder_move, debug_info = self.get_best_and_ponder_move()
        best_move, self.ponder_move = self.get_best_and_ponder_move()
        t_end_think = time.time()
        delta_t_ms = int((t_end_think - t_start_best_move) * 1000)

        remain_time = time_self_ms - delta_t_ms
        if self.ponder_move:
            # start pondering
            self.pondering = True
            self._send_command(f"position fen {fen} moves {best_move} {self.ponder_move}")
            wtime = remain_time if color == "white" else time_opponent_ms
            btime = time_opponent_ms if color == "white" else remain_time
            self._send_command(f"go ponder wtime {wtime} btime {btime}")

        # # print debug info
        # ss = f"step={str(move_count).rjust(3)}, last_move={str(last_move).rjust(4)}, "
        # ss += f"time_opponent_ms={str(time_opponent_ms).rjust(5)}, time_self_ms={str(time_self_ms).rjust(5)} / "
        # ss += f"[ponderhit] " if ponderhit else "            "
        # ss += f"duration_ms: {str(delta_t_ms).rjust(3)}, best_move: {best_move}, ponder_move: {self.ponder_move} / "
        # ss += debug_info
        # print(ss)

        return best_move


engine = None


def chess_bot(obs):
    global engine
    if engine is None:
        engine = ChessEngine("/kaggle_simulations/agent/cfish")

    best_move = engine.get_best_move(obs)

    return best_move
