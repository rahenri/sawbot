import random
import utils

DIRECTIONS = {
    'right': (1, 0),
    'down': (0, 1),
    'left': (-1, 0),
    'up': (0, -1),
}

FIELD_WIDTH = 16
FIELD_HEIGHT = 16
FIELD_SIZE = FIELD_WIDTH * FIELD_HEIGHT

def move(position, direction):
  dx, dy = DIRECTIONS[direction]
  x, y = position
  return (dx+x, dy+y)


def Collided(prev1, after1, prev2, after2):
  return (after1 == after2) or (prev1 == after2 and prev2 == after1)

def RandomBot():
    return (random.randint(1, 6), random.randint(1, 14))


class GameState:

  def __init__(self, names = (1,2)):
    self.round = 1
    self.names = names
    self.dead = [False, False]

    self.walls = [[0]*FIELD_WIDTH for _ in range(FIELD_HEIGHT)]

    bot1 = RandomBot()
    (x1, y1) = bot1
    bot2 = (15-x1,y1)
    self.bots = [bot1, bot2]

  @classmethod
  def FromJSon(cls, static_data, mutable_data = None):
    names = static_data['names']
    state = GameState(names)

    if mutable_data:
      state.UpdateFromJSon(mutable_data)

    return state

  def pos_id(self, pos):
    x, y = pos
    return x + y * self.width

  def UpdateFromJSon(self, mutable_data):
    data = mutable_data
    self.bots = data['bots']
    self.round = data['round']
    self.dead = data['dead']

  def _RandomCell(self, wall=0):
    while True:
      x = random.randint(0, self.width-1)
      y = random.randint(0, self.height-1)
      coord = (x, y)
      if self.walls[y][x] != wall:
        continue
      if coord in self.bots:
        continue
      return coord

  def StaticJSon(self):
    return {
        'names': self.names,
    }

  def JSon(self):
    return {
        'bots': self.bots,
        'round': self.round,
        'dead': self.dead,
    }

  def JSonFull(self):
    out = self.JSon()
    out.update(self.StaticJSon())
    return out

  def ValidModes(self, idx):
    bx, by = self.bots[idx]
    out = []
    for name, (dx, dy) in DIRECTIONS.items():
      next = (bx+dx, by+dy)
      nx, ny = next
      if nx < 0 or nx >= self.width or ny < 0 or ny >= self.height:
        continue
      if self.walls[ny][nx] != 1:
        continue
      out.append(name)
    return out

  def Step(self, actions):
    assert len(actions) == len(self.bots)

    # Update bot positions
    assert(len(actions) == len(self.bots))
    for idx, action in enumerate(actions):
        src = self.bots[idx]
        x1, y1 = src
        if not action in DIRECTIONS:
          raise ValueError('Invalid bot action: {}'.format(action))
        dx, dy = DIRECTIONS[action]
        x2 = x1 + dx
        y2 = y1 + dy
        if x2 < 0 or y2 < 0 or x2 >= FIELD_WIDTH or y2 >= FIELD_HEIGHT:
            self.dead[idx] = True
        elif self.walls[y2][x2] == 1:
            self.dead[idx] = True
        else:
            self.walls[y2][x2] = 1
        self.bots[idx] = (x2, y2)

    if self.bots[0] == self.bots[1]:
        self.dead[0] = True
        self.dead[1] = True

    self.round += 1
    
  def FieldStr(self):
    field = []
    for row in self.walls:
      str_row = []
      for col in row:
        str_row.append('x' if col else '.')
      field.append(str_row)
    for i, (x, y) in enumerate(self.bots):
      field[y][x] = str(i)

    flat_field = [cell for row in field for cell in row]
    return ','.join(flat_field)

  def PrettyPrint(self):
    field = []
    field.append(['+'] + (['-']*FIELD_WIDTH) + ['+'])
    for row in self.walls:
      str_row = ['|']
      for col in row:
        str_row.append('X' if col else ' ')
      str_row.append('|')
      field.append(str_row)
    field.append(['+'] + (['-']*FIELD_WIDTH) + ['+'])
    for i, (x, y) in enumerate(self.bots):
      field[y+1][x+1] = utils.ColorFormat(str(i), 'yellow')
    print('Round: {}'.format(self.round))

    for row in field:
      print(''.join(row))


  def IsDone(self):
      return any(self.dead)

  # Returns
  # -1 - no winner
  # 0 - player1 is winner
  # 1 - player2 is winner
  # Raise exception if game not done yet
  def Winner(self):
      if self.dead[0] and self.dead[1]:
          return -1
      if self.dead[0]:
          return 1
      if self.dead[1]:
          return 0
      raise ValueError('Not done yet')
