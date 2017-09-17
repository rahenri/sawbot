#!/usr/bin/env python3
# coding=utf-8

from __future__ import division

import argparse
import json
import logging
import math
import multiprocessing
import os
import random
import sys
import time
import traceback
import game_state
import utils
import serialize

from subprocess import Popen, PIPE, STDOUT

from datetime import datetime

class Estimator:
  def __init__(self):
    self.elements = []

  def add(self, item):
    self.elements.append(item)

  def mean(self):
    l = len(self)
    if l == 0:
      return 0
    return sum(self.elements) / l

  def mean(self):
    l = len(self)
    return sum(self.elements) / l if l else 0.0

  def variance(self):
    l = len(self)
    if l == 0:
      return 0.0
    m = self.mean()
    out = 0.0
    for i in self.elements:
      d = (i - m)
      out += d * d
    return out / l

  def confidence(self):
    return 1.96 * math.sqrt(self.variance() / len(self)) if len(self) else 0.0

  def __len__(self):
    return len(self.elements)

class BotInfo:
  def __init__(self, cmd, identity, log_dir, display_name = None, weights = None):
    self.cmd = cmd
    self.identity = identity
    self.display_name = display_name or cmd
    self.log_dir = log_dir
    self.weights = weights

  def Run(self, round_id, side):
    extra_commands = []
    if self.weights:
      values = ' '.join('{} {}'.format(k, v) for k, v in self.weights)
      extra_commands.append('weights {}'.format(values))
    log_path = '/dev/null'
    if self.log_dir:
      log_path = '{}/bot.stderr.{}.{}.{}'.format(self.log_dir, self.identity, round_id + 1, side)
    with open(log_path, 'w') as stderr:
      proc = Popen(self.cmd, shell=True, stdout=PIPE, stdin=PIPE, stderr=stderr, universal_newlines=True, bufsize=1)
      return BotProc(self.identity, self.display_name, proc, extra_commands)


  def __cmp__(self, other):
    return cmp(self.identity, other.identity)

  def __repr__(self):
    return self.display_name


class BotProc:
  def __init__(self, identity, cmd, proc, extra_commands = None):
    self.identity = identity
    self.cmd = cmd
    self._proc = proc
    self._timebank = 10000
    self._extra_commands = extra_commands

  def InitTimer(self, timebank, time_per_move):
    self.timebank = timebank
    self.time_per_move = time_per_move

  def SendSettings(self, bot_id):
    settings = (
        'settings timebank {timebank}\n'
        'settings time_per_move {time_per_move}\n'
        'settings player_names player0,player1\n'
        'settings your_bot player{bot_id}\n'
        'settings your_botid {bot_id}\n'
        'settings field_width {field_width}\n'
        'settings field_height {field_height}\n').format(
            bot_id=bot_id,
            timebank=self.timebank,
            time_per_move=self.time_per_move,
            field_width=game_state.FIELD_WIDTH,
            field_height=game_state.FIELD_HEIGHT)

    self._proc.stdin.write(settings)

    if self._extra_commands:
      for cmd in self._extra_commands: self._proc.stdin.write(cmd + '\n')


  def SendUpdate(self, round_num, field):
    start = time.time()
    updates = (
        'update game round {round}\n'
        'update game field {field}\n'
        'action move {timebank}\n').format(
            round=round_num,
            field=field,
            timebank=int(self.timebank))

    self._proc.stdin.write(updates)

    while True:
      line = self._proc.stdout.readline()
      if not line:
        print('Bot crashed')
        line = 'die'
        break
      line = line.strip()
      if line:
        break

    end = time.time()
    self.timebank += self.time_per_move - (end - start) * 1000.0

    return line

  def close(self):
    self._proc.stdin.close()
    self._proc.wait()

def RatingDelta(wins, draws, losses):
  total = wins+losses+draws
  if total == 0:
    return 0
  if wins+draws == 0 and losses>0:
    return -1000
  if losses+draws == 0 and wins>0:
    return 1000

  score = (wins + draws * 0.5) / total
  return int(round(-400 * math.log10(1.0 / score - 1.0)))

class Score:
  def __init__(self, left, right):
    self.wins = 0
    self.loses = 0
    self.draws = 0
    self.left = left
    self.right = right
    self.estimator = Estimator()
    self.estimator_nodraw = Estimator()

  def add(self, outcome):
    if outcome == 1:
      self.wins += 1
      self.estimator.add(1.0)
      self.estimator_nodraw.add(1.0)
    elif outcome == -1:
      self.loses += 1
      self.estimator.add(0.0)
      self.estimator_nodraw.add(0.0)
    elif outcome == 0:
      self.draws += 1
      self.estimator.add(0.5)
    else:
      raise ValueError('Unexpected outcome value')

  def PrintSummary(self):
    score = self.estimator.mean()
    conf = self.estimator.confidence()
    score_nodraw = self.estimator_nodraw.mean()
    conf_nodraw = self.estimator_nodraw.confidence()
    rating = RatingDelta(self.wins, self.draws, self.loses)
    total = self.wins+self.loses+self.draws

    print('Base({}):{} Test({}):{} Draws:{} Total:{} Score:{:.1f}±{:.1f}% ScoreNoDraw:{:.1f}±{:.1f}% Rating:{:+d}'.format(
      self.left.display_name, self.loses, self.right.display_name, self.wins, self.draws, total, score*100, conf*100, score_nodraw*100, conf_nodraw*100, rating))


class ScoreBoard:
  def __init__(self):
    self._score = {}

  def _get(self, b1, b2):
    key = (b1.identity, b2.identity)
    out = self._score.get(key, None)
    if not out:
      out = Score(b1, b2)
      self._score[key] = out
    return out

  def add(self, b1, b2, result):
    if b1.identity > b2.identity:
      b1, b2 = b2, b1
    score = self._get(b1, b2)
    if result == b1.identity:
      score.add(-1)
    elif result == b2.identity:
      score.add(1)
    elif result == -1:
      score.add(0)
    else:
      raise ValueError('Unexpected result %s' % str(result))

  def PrintSummary(self):
    print('-' * 80)
    for key in sorted(self._score):
      score = self._score[key]
      score.PrintSummary()
    sys.stdout.flush()


def ParseBots(args, config):
  if args.bots:
    bots = []
    identity = 1
    for cmd in args.bots:
      bots.append(BotInfo(cmd, identity, args.logdir))
      identity += 1

    games = []
    round_id = 0
    for _ in range((args.count+1) // 2):
      g = []
      j = len(bots) - 1
      for i in range(len(bots)-1):
        g.append((bots[i], bots[j], round_id))
        g.append((bots[j], bots[i], round_id+1))
        round_id += 2
      random.shuffle(g)
      games.extend(g)
    if len(games) > args.count:
      games = games[:args.count]
  elif config:
    bots = {}
    identity = 1
    for bot in config['bots']:
      cmd = bot['cmd']
      name = bot['name']
      info = BotInfo(cmd, identity, args.logdir, name)
      if name in bots:
        raise(ValueError('Duplicated bot name: {}'.format(name)))
      bots[name] = info
      identity += 1


    raw_pairs = config['pairs']
    pairs = []
    for pair in raw_pairs:
      p1 = bots[pair[0]]
      p2 = bots[pair[1]]
      pairs.append((p1, p2))

    games = []
    round_id = 0
    for _ in range((args.count+1) // 2):
      g = []
      for pair in pairs:
        g.append((pair[0], pair[1], round_id))
        g.append((pair[1], pair[0], round_id+1))
        round_id += 2
      random.shuffle(g)
      games.extend(g)
  else:
    raise(ValueError("No bots or config provided"))

  return games

def PinCPU(counter):
  with counter.get_lock():
    cpu_id = counter.value
    counter.value += 1

  pid = os.getpid()
  os.system('taskset -pc {} {}'.format(cpu_id, pid))
  print('Pinned pid {} to cpu {}'.format(pid, cpu_id))


class WeightInfo:
  def __init__(self, name, minimum, maximum):
    self.name = name
    self.minimum = minimum
    self.maximum = maximum

  def RandomValue(self):
    return random.randint(self.minimum, self.maximum)

class FakePool:
    def __init__(self):
        pass

    def imap_unordered(self, callback, iterator, chunksize=1):
        for v in iterator:
            yield callback(v)

def main(args):
  if args.config:
    with open(args.config, 'r') as f:
      config = json.loads(f.read())
  else:
    config = None


  # cpu_counter= multiprocessing.Value('i', lock=True)
  # pool = multiprocessing.Pool(
  #     processes=args.workers, initializer=PinCPU, initargs=(cpu_counter,)) 
  pool = multiprocessing.Pool(processes=args.workers) 
  # pool = FakePool()

  hist_file = datetime.now().strftime('games-%Y%m%d-%H%M%S.txt')
  hist_path = os.path.join(args.history, hist_file)

  if args.action == 'eval':
    print('Evaluating...')
    games = ParseBots(args, config)
    score_board = ScoreBoard()
    with open(hist_path, 'w') as hist:
      for b1, b2, result, history in pool.imap_unordered(OneRound, games, chunksize=1):
        score_board.add(b1, b2, result)
        # print summary
        score_board.PrintSummary()
        hist.write(history)
        hist.write('\n')
        hist.flush()
  elif args.action == 'train':
    print('Training...')
    weights = []
    for info in config.get('trainable_weights', []):
      weights.append(WeightInfo(info['name'], info['min'], info['max']))

    train_bot_name = config['train_bot']
    train_bot_config = None
    for info in config['bots']:
      if info['name'] == train_bot_name:
        train_bot_config = info
        break

    if train_bot_config is None:
      raise ValueError('Bot {} not defined'.format(train_bot_name))

    games = []
    train_bot_cmd = train_bot_config['cmd']
    for i in range(args.count):
      w = [(w.name,w.RandomValue()) for w in weights]
      base = BotInfo(train_bot_cmd, 1, args.logdir, 'base')
      test = BotInfo(train_bot_cmd, 2, args.logdir, 'test', weights=w)

      if i % 2 == 0:
        game = [base, test, i]
      else:
        game = [test, base, i]

      games.append(game)

    with open(hist_path, 'w') as hist, open('train_result.csv', 'w') as train_result:
      header = ','.join([w.name for w in weights] + ['result'])
      train_result.write(header +'\n')
      for i, (b1, b2, result, history) in enumerate(pool.imap_unordered(OneRound, games, chunksize=1)):

        score = 0
        if result == 1:
          score = -1
        elif result == 2:
          score = 1
        elif result == -1:
          score = 0
        else:
          raise RuntimeError('Unexpected result value: {}'.format(result))

        w = b1.weights if b1.identity == 2 else b2.weights

        values = tuple(i[1] for i in w) + (score,)
        line = ','.join(str(v) for v in values)
        train_result.write(line + '\n')
        train_result.flush()

        line = ' '.join('{: 2}'.format(v) for v in values)
        print('{}: {}'.format(i, line))

        hist.write(history)
        hist.write('\n')
        hist.flush()

  else:
    raise ValueError('Invalid action: {}'.format(args.action))


def OneRound(params):
  bot1, bot2, round_id = params

  bots = []
  try:

    positions = []

    # Get robots who are fighting (player1, player2)
    bots = [bot1.Run(round_id, 1), bot2.Run(round_id, 2)]

    bots[0].InitTimer(args.timebank, args.time_per_move)
    bots[1].InitTimer(args.timebank, args.time_per_move)

    # Simulate game init input
    bots[0].SendSettings('0')
    bots[1].SendSettings('1')

    move = 1
    names = (bot1.display_name, bot2.display_name)
    state = game_state.GameState(names)
    result = 0
    if args.verbose:
      state.PrettyPrint()

    game_info = serialize.GameInfoBuilder(bot1, bot2, state)
    game_info.AddRound(state, [])
    while True:
      # Send inputs to bot
      actions = []
      for bot in bots:
        ac = bot.SendUpdate(state.round, state.FieldStr())
        actions.append(ac)
      state.Step(actions)
      if args.verbose:
        state.PrettyPrint()
      game_info.AddRound(state, actions)

      if state.IsDone():
        winner = state.Winner()
        if args.verbose:
          if winner != -1:
            print(utils.ColorFormat('Winner: Player {}'.format(winner), 'yellow'))
          else:
            print(utils.ColorFormat('Draw', 'yellow'))
        result = -1 if (winner == -1) else bots[winner].identity
        game_info.SetResult(result)
        break

  except KeyboardInterrupt as exp:
    raise RuntimeError("Keyboard Interrupt in child")
  except Exception as exp:
    traceback.print_exc()
    logging.error(exp)
    raise
  finally:
    for b in bots:
      b.close()

  return bot1, bot2, result, game_info.JSon()

if __name__ == '__main__':
  workers = multiprocessing.cpu_count() // 2
  parser = argparse.ArgumentParser(description='Run two bots againts each other.')
  parser.add_argument('bots', nargs='*', help='The bots to be tested')
  parser.add_argument('--action', default='eval', help='Action to take, either eval or train')
  parser.add_argument('--count', type=int, default=1000, help='Number of times to run the bots (Default: 1)')
  parser.add_argument('--time-per-move', type=int, default=100, help='Milliseconds added to time bank each turn (Default: 100)')
  parser.add_argument('--timebank', type=int, default=10000, help='Milliseconds initial time in the time bank (Default: 10000)')
  parser.add_argument('--workers', type=int, default=workers, help='Number of parallel workers (Default: 1)')
  parser.add_argument('--history', default='history', help='Directory to store game history')
  parser.add_argument('--config', help='Config file with bots to test')
  parser.add_argument('--verbose', help='Print games to output', action='store_true')
  parser.add_argument('--logdir', help='Write bots stderr to this directory')
  args = parser.parse_args()
  main(args)
