all:
	g++ -o tracker tracker.cpp
	g++ -o peer peers.cpp
	mkdir -p peer_storage
clean:
	rm tracker peer
format:
	rm ./peer_storage/*