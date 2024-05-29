from requests import get
from time import time
from random import randint, seed

seed(time())

beg = time()

for _ in range(100):
    a = randint(0, 100)
    b = randint(0, 100)
    url = f'http://localhost:8888/add?a={a}&b={b}'
    res = get(url)
    print(res.text.split('\r\n')[1][3:-9])

print(time() - beg)
