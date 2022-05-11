def Settings( **kwargs ):
  return {
    'flags': ['-x', 'c++', '-Wall', '-pedantic', '-Isrc',
    '-IConcurrent', '-Iconcurrent', '-IuSockets/src/',
    '-std=c++2a', '-I/usr/include'],
  }
