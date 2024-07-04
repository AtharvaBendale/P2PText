all:
	g++ -o tracker tracker.cpp
	g++ -o peer peers.cpp
clean:
	rm tracker peer
format:
	rm ./peer_storage/*