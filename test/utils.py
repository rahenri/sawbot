COLOR_CODE = {
    'black': 30,
    'red': 31,
    'green': 32,
    'yellow': 33,
    'blue': 34,
    'magent': 35,
    'cyan': 36,
    'white': 37,
}

def ColorFormat(text, color):
  return '\033[1;{code}m{text}\033[0m'.format(code=COLOR_CODE[color], text=text)

