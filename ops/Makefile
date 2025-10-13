# CAN-RTOS-Sim Makefile
# Quick commands for common operations

.PHONY: help up down logs inject clean build status can-monitor

help: ## Show this help message
	@echo "CAN-RTOS-Sim Commands:"
	@echo ""
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-15s\033[0m %s\n", $$1, $$2}'

build: ## Build all Docker images
	docker compose -f compose.yml build

up: ## Start all services
	docker compose -f compose.yml up -d bus
	sleep 2
	docker compose -f compose.yml up -d node_a node_b server panel

down: ## Stop all services
	docker compose -f compose.yml down

restart: down up ## Restart all services

logs: ## Show logs from all services
	docker compose -f compose.yml logs -f

logs-node-a: ## Show logs from node A only
	docker compose -f compose.yml logs -f node_a

logs-node-b: ## Show logs from node B only
	docker compose -f compose.yml logs -f node_b

logs-server: ## Show logs from server only
	docker compose -f compose.yml logs -f server

status: ## Show service status
	docker compose -f compose.yml ps

can-monitor: ## Monitor CAN traffic (requires bus service running)
	docker compose -f compose.yml exec bus candump -l vcan0

inject-drop: ## Inject drop fault (requires server running)
	curl -X POST http://localhost:8000/faults/inject -H "Content-Type: application/json" -d '{"type": "drop", "durationMs": 5000}'

inject-delay: ## Inject delay fault
	curl -X POST http://localhost:8000/faults/inject -H "Content-Type: application/json" -d '{"type": "delay", "durationMs": 3000, "delayMs": 100}'

inject-flood: ## Inject flood fault
	curl -X POST http://localhost:8000/faults/inject -H "Content-Type: application/json" -d '{"type": "flood", "durationMs": 3000, "rate": 1000}'

inject-reset: ## Reset node A
	curl -X POST http://localhost:8000/faults/inject -H "Content-Type: application/json" -d '{"type": "reset"}'

clean: ## Remove all containers and images
	docker compose -f compose.yml down -v --rmi all

dashboard: ## Open dashboard in browser
	open http://localhost:3000

health-check: ## Check if vcan0 interface exists
	docker compose -f compose.yml exec bus ip link show vcan0

frame-count: ## Count frames in last 10 seconds
	docker compose -f compose.yml exec bus timeout 10 candump -l vcan0 | wc -l

