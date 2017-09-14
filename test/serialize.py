import json

class GameInfoBuilder:
  def __init__(self, bot1, bot2, init_state):
    b1 = {
        'cmd': bot1.cmd,
        'id': bot1.identity,
    }
    b2 = {
        'cmd': bot2.cmd,
        'id': bot2.identity,
    }
    self.rounds = []
    self.game = {
        'bot1': b1,
        'bot2': b2,
        'init_state': init_state.StaticJSon(),
        'rounds': self.rounds,
    }

  def AddRound(self, state, moves):
    self.rounds.append({
      'state': state.JSon(),
      'last_moves': moves,
    })

  # result is 1 for bot 1, 2 for bot 2, or 0 for draw.
  def SetResult(self, result):
    self.game['result'] = result

  def JSon(self):
    return json.dumps(self.game, separators=(',', ':'))

