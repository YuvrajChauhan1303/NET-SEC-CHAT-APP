build:
	docker build -t chat-server -f Dockerfile.server .
	docker build -t chat-client -f Dockerfile.client .

server-only:
	docker compose up server

client-only:
	docker compose run --rm client

down:
	docker compose down --remove-orphans

restart:
	docker compose down --remove-orphans
	docker compose up server