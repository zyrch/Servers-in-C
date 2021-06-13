# Concurrent servers in c

Imlementation of concurrent severs in c from scratch using [this awesome blog](https://eli.thegreenplace.net/2017/concurrent-servers-part-1-introduction/) as a template.

### Project Structure

```bash
.
├── readme.md
├── concurrent
│   ├── thread-pool
│   │   ├── makefile
│   │   ├── readme.md
│   │   └── server.c
│   └── threads
│       ├── makefile
│       ├── readme.md
│       └── server.c
├── event-driven
│   ├── epoll
│   │   ├── makefile
│   │   ├── readme.md
│   │   └── server.c
│   └── select
│       ├── makefile
│       ├── readme.md
│       └── server.c
├── file
├── headers
│   ├── thpool.h
│   └── utils.h
├── libuv
│   ├── makefile
│   └── server.c
├── sequential
│   ├── makefile
│   ├── readme.md
│   └── server.c
├── simple-client.py
└── src
    ├── thpool.c
    └── utils.c

10 directories, 24 files

```

### Dependencies 

  - [socket](https://man7.org/linux/man-pages/man2/socket.2.html)
  - [libuv 1.41.1-dev](https://github.com/libuv/libuv/tree/v1.x)
  - event-driven:
    - [epoll](https://man7.org/linux/man-pages/man7/epoll.7.html)
    - [select](https://man7.org/linux/man-pages/man2/select.2.html)
  - concurrent:
    - [thread-pool](https://github.com/Pithikos/C-Thread-Pool)
    - [pthread](https://man7.org/linux/man-pages/man7/pthreads.7.html)
    
### How to Run

  - Server:
    ```bash
    make && ./server
    ```
  - client:
    ```bash
    python3 simple-client.py localhost -n [NUM_OF_CLIENTS] 8000
    ```




