#
# Testcases for systems programming hw2.
# Author: Chi-Sheng Liu
#
# ========================================

import unittest
import subprocess
from itertools import combinations
from collections import Counter

# ========================================

N_HOST = 10
N_PLAYER = 12

# ========================================


class AuctionSystem:
    def run(self, n_player):
        scores = [None] + [0] * n_player
        for players in combinations(range(1, n_player + 1), 8):
            ranking = self.host(0, players)
            for player_id, rank in ranking:
                scores[player_id] += (8 - rank)
        return "".join([f"{i+1} {x}\n" for i, x in enumerate(scores[1:])])

    def host(self, depth, players):
        if depth == 0:
            first_part = self.host(1, players[:4])
            second_part = self.host(1, players[4:])
            scores = Counter([a[0] if a[1] > b[1] else b[0]
                              for a, b in zip(first_part, second_part)])
            for player_id in players:
                if player_id not in scores:
                    scores[player_id] = 0
            sorted_scores = sorted(scores.values(), reverse=True)
            ranking = sorted([(k, sorted_scores.index(v) + 1)
                              for k, v in scores.items()])
            return ranking

        if depth == 1:
            first_part = self.host(2, players[:2])
            second_part = self.host(2, players[2:])
            return [a if a[1] > b[1] else b
                    for a, b in zip(first_part, second_part)]

        if depth == 2:
            player1_bids = self.player(players[0])
            player2_bids = self.player(players[1])
            ret = list()
            for a, b in zip(player1_bids, player2_bids):
                if a > b:
                    ret.append((players[0], a))
                else:
                    ret.append((players[1], b))
            return ret

        raise ValueError("Invalid depth")

    def player(self, player_id):
        bid_list = [20, 18, 5, 21, 8, 7, 2, 19, 14,
                    13, 9, 1, 6, 10, 16, 11, 4, 12, 15, 17, 3]
        return [bid_list[player_id + i - 2] * 100 for i in range(1, 11)]


class TestAuctionSystem(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        subprocess.run(["git", "checkout", "host"])
        subprocess.run(["git", "checkout", "player"])
        subprocess.run(["mv", "host", "TA_host"])
        subprocess.run(["mv", "player", "TA_player"])
        subprocess.run(["make"])
        subprocess.run(["mv", "host", "my_host"])
        subprocess.run(["mv", "player", "my_player"])

        auction_ans = dict()
        auction_system = AuctionSystem()
        for n_player in range(8, N_PLAYER + 1):
            auction_ans[n_player] = auction_system.run(n_player).encode()
        cls.auction_ans = auction_ans

    @classmethod
    def tearDownClass(cls):
        subprocess.run(["rm", "my_host"])
        subprocess.run(["rm", "my_player"])
        subprocess.run(["mv", "TA_host", "host"])
        subprocess.run(["mv", "TA_player", "player"])

    def setUp(self):
        pass

    def tearDown(self):
        subprocess.run(["rm", "-f", "host"])
        subprocess.run(["rm", "-f", "player"])

    def test_1_1__player(self):
        """ Test player program. """
        # player programs do not fork and exec, so we don't need to rename them
        for i in range(1, N_PLAYER + 1):
            TA_output = subprocess.check_output(["./TA_player", str(i)])
            my_output = subprocess.check_output(["./my_player", str(i)])
            self.assertEqual(TA_output, my_output)

    def test_2_1__leaf_host(self):
        """ Test host program (depth = 2) """
        subprocess.run(["ln", "-s", "TA_player", "player"])

        n_players_assign = 2
        text_input = ""
        for x in combinations(range(1, N_PLAYER + 1), n_players_assign):
            text_input += (" ".join(["{}"] * n_players_assign) + "\n"
                           ).format(*x)
        text_input += " ".join(["-1"] * n_players_assign) + "\n"

        args = ("1", "3427", "2")
        subprocess.run(["ln", "-sf", "TA_host", "host"])
        TA_output = subprocess.check_output(
            ["./host", *args], input=text_input, text=True)
        subprocess.run(["ln", "-sf", "my_host", "host"])
        my_output = subprocess.check_output(
            ["./host", *args], input=text_input, text=True)
        self.assertEqual(TA_output, my_output)

    def test_2_2__mid_host(self):
        """ Test host program (depth = 1) """
        subprocess.run(["ln", "-s", "TA_player", "player"])

        n_players_assign = 4
        text_input = ""
        for x in combinations(range(1, N_PLAYER + 1), n_players_assign):
            text_input += (" ".join(["{}"] * n_players_assign) + "\n"
                           ).format(*x)
        text_input += " ".join(["-1"] * n_players_assign) + "\n"

        args = ("1", "3427", "1")
        subprocess.run(["ln", "-sf", "TA_host", "host"])
        TA_output = subprocess.check_output(
            ["./host", *args], input=text_input, text=True)
        subprocess.run(["ln", "-sf", "my_host", "host"])
        my_output = subprocess.check_output(
            ["./host", *args], input=text_input, text=True)
        self.assertEqual(TA_output, my_output)

    def test_2_3__root_host(self):
        """ Test host program (depth = 0) """
        subprocess.run(["ln", "-s", "TA_player", "player"])
        subprocess.run(["mkfifo", "fifo_0.tmp", "fifo_1.tmp"])

        n_players_assign = 8
        text_input = ""
        for x in combinations(range(1, N_PLAYER + 1), n_players_assign):
            text_input += (" ".join(["{}"] * n_players_assign) + "\n"
                           ).format(*x)
        text_input += " ".join(["-1"] * n_players_assign) + "\n"

        args = ("1", "3427", "0")

        subprocess.run(["ln", "-sf", "TA_host", "host"])
        with subprocess.Popen("./host " + " ".join(args) + " 3<> fifo_1.tmp",
                              shell=True) as host, \
                open("fifo_0.tmp", "r") as fifo_read, \
                open("fifo_1.tmp", "wb", 0) as fifo_write:
            fifo_write.write(text_input.encode())
            TA_output = fifo_read.read()

        subprocess.run(["ln", "-sf", "my_host", "host"])
        with subprocess.Popen(["./host", *args]) as host, \
                open("fifo_0.tmp", "r") as fifo_read, \
                open("fifo_1.tmp", "wb", 0) as fifo_write:
            fifo_write.write(text_input.encode())
            my_output = fifo_read.read()

        self.assertEqual(TA_output, my_output)
        subprocess.run(["rm", "fifo_0.tmp", "fifo_1.tmp"])

    def test_3_1__auction_system(self):
        """ Test auction system (with TA's host and player) """
        subprocess.run(["ln", "-s", "TA_player", "player"])
        subprocess.run(["ln", "-sf", "TA_host", "host"])

        auction_ans = type(self).auction_ans
        for n_host in range(1, N_HOST + 1):
            for n_player in range(8, N_PLAYER + 1):
                output = subprocess.check_output(
                    ["bash", "auction_system.sh", str(n_host), str(n_player)])
                self.assertEqual(output, auction_ans[n_player])

    def test_3_2__auction_system(self):
        """ Test auction system (with our host and player) """
        subprocess.run(["ln", "-s", "my_player", "player"])
        subprocess.run(["ln", "-sf", "my_host", "host"])

        auction_ans = type(self).auction_ans
        for n_host in range(1, N_HOST + 1):
            for n_player in range(8, N_PLAYER + 1):
                output = subprocess.check_output(
                    ["bash", "auction_system.sh", str(n_host), str(n_player)])
                self.assertEqual(output, auction_ans[n_player])

# ========================================


if __name__ == "__main__":
    unittest.main(verbosity=2)
