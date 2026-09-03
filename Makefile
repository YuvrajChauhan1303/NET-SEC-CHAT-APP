ca:
	docker compose up cert-auth

server-only:
	docker compose up server

client-only:
	docker build -t chat-client -f Dockerfile.client .
	docker compose run --rm client ./client/client server 8080

build:
	docker build -t chat-ca -f Dockerfile.ca .
	docker build -t chat-server -f Dockerfile.server .
	docker build -t chat-client -f Dockerfile.client .

down:
	docker compose down --remove-orphans

restart:
	docker compose down --remove-orphans
	docker compose up cert-auth server

rebuild:
	docker compose down --remove-orphans
	docker build -t chat-ca -f Dockerfile.ca .
	docker build -t chat-server -f Dockerfile.server .
	docker build -t chat-client -f Dockerfile.client .
	docker compose up cert-auth server