FROM alpine:3.11.6 AS BUILDER

WORKDIR /lox/

RUN [ "apk", "add", "--no-cache", "gcc", "openjdk7", "make", "python3", \
      "git", "npm" , "libc-dev", "llvm"  ]
RUN [ "npm", "install", "-g", "sass" ]

ENV JAVA_HOME=/usr/lib/jvm/java-1.7-openjdk
ENV PATH="$JAVA_HOME/bin:${PATH}"
COPY . .

RUN [ "make", "setup" ]
RUN [ "make" ]

# FROM alpine:3.11.6 AS RUNNER
# 
# COPY --from=BUILDER /lox/ /lox/
