import time

def fib(n):
    if n < 2:
        return n
    return fib(n - 2) + fib(n - 1)

start = time.time()
print(fib(35))
print(time.time() - start)