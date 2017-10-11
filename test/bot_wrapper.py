from subprocess import Popen, PIPE, STDOUT

import game_state
import time
import json

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

    def _readline(self):
        while True:
            line = self._proc.stdout.readline()
            if not line:
                print('Bot crashed')
                return None
            line = line.strip()
            if line:
                return line

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

        line = self._readline()
        if line is None:
            line = 'die'

        end = time.time()
        self.timebank += self.time_per_move - (end - start) * 1000.0

        return line

    def gen_features(self, round_num, field):
        updates = (
                'update game round {round}\n'
                'update game field {field}\n'
                'features\n').format(
                        round=round_num,
                        field=field)
        self._proc.stdin.write(updates)
        line = self._readline()
        if line is None:
            raise RuntimeError('Bot died')
        return json.loads(line)

    def close(self):
        self._proc.stdin.close()
        self._proc.wait()

