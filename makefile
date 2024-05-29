# all: complie run


# complie:
# 	g++ -Wall src/Locker.cpp src/Semaphore.cpp src/HTTP.cpp src/main.cpp -o bin/main
	
# ## 运行
# run:
# 	bin/main

# ## 清理
# clean:
# 	rm -f obj/*.o bin/*


all: try run

try:
	g++ -Wall src/Logger.cpp src/Try.cpp -o bin/try
run:
	bin/try

