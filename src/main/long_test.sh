#!/bin/bash
# long_test.sh - Run and monitor for extended period

echo "Starting long-term memory stability test..."
echo "Log file: memory_log_$(date +%Y%m%d_%H%M%S).txt"
echo "This test will run for 24 hours..."
echo "Press Ctrl+C to stop"

./server 8888 &
SERVER_PID=$!

sleep 1

./client 192.168.1.16 8888 tom &
CLIENT1_PID=$!

./client 192.168.1.16 8888 jerry &
CLIENT2_PID=$!

echo "PIDs: Server=$SERVER_PID, Client1=$CLIENT1_PID, Client2=$CLIENT2_PID"
echo ""

# Log memory every 5 minutes for 24 hours
for i in $(seq 1 288); do  # 288 * 5 minutes = 24 hours
    echo "$(date '+%Y-%m-%d %H:%M:%S') - Sample $i"
    ps -o pid,rss,vsz,comm -p $SERVER_PID,$CLIENT1_PID,$CLIENT2_PID
    echo "---"
    sleep 300  # 5 minutes
done

kill $SERVER_PID $CLIENT1_PID $CLIENT2_PID
