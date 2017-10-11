#!/usr/bin/env python3
# coding=utf-8

from __future__ import division

import argparse
import json
import game_state
import serialize
import bot_wrapper
import pandas as pd

from datetime import datetime

class DataBuilder:
    def __init__(self):
        self.data = {}

    def append(self, data):
        for k, v in data.items():
            if not k in self.data:
                self.data[k] = []
            self.data[k].append(v)

    def build(self):
        return pd.DataFrame(self.data)


def main(args):
    bot_info = bot_wrapper.BotInfo(args.bot, 1, None)
    bot = bot_info.Run(0, 0)
    print('Generating features...')
    builder = DataBuilder()
    for hist in args.history:
        with open(hist, 'r') as history_file:
            for line in history_file:
                history = json.loads(line)
                bot1 = history['bot1']['id']
                bot2 = history['bot2']['id']
                init_state = history['init_state']
                winner = history['result']
                if winner == bot1:
                    result = 1
                elif winner == bot2:
                    result = 0
                elif winner == -1:
                    result = 0.5
                else:
                    raise RuntimeError('Invalid result')

                for game_round in history['rounds'][:-1]:
                    state = game_state.GameState.FromJSon(init_state, game_round['state'])
                    features = bot.gen_features(state.round, state.FieldStr())
                    features['result'] = result
                    builder.append(features)
    df = builder.build()
    print(df)
    print(df.describe())

    df.to_csv('training_set.csv.gz', index=False, compression='gzip')



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run two bots againts each other.')
    parser.add_argument('bot', nargs=1, help='The bot command to generate features')
    parser.add_argument('history', nargs='+', help='The history files to process')
    args = parser.parse_args()
    main(args)
