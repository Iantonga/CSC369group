import string
import random
import os

def id_generator(size=2, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))

for i in range(100):
    directory = id_generator()
    if not os.path.exists(directory):
        os.makedirs(directory)
