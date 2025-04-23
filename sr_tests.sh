#!/bin/bash

# Create results directory
mkdir -p test_results

# Compile SR protocol implementation
echo -e "Compiling SR protocol implementation..."
gcc -o sr sr.c emulator.c -Wall

# Function to run a test and save results with parameters
run_test() {
    test_name=$1
    num_msgs=$2
    loss_prob=$3
    corrupt_prob=$4
    direction=$5
    lambda=$6
    trace=$7
    
    output_file="test_results/${test_name}.txt"
    
    # Write test parameters to the output file
    echo "=== TEST: ${test_name} ===" > $output_file
    echo "Number of messages: $num_msgs" >> $output_file
    echo "Loss probability: $loss_prob" >> $output_file
    echo "Corruption probability: $corrupt_prob" >> $output_file
    echo "Direction (0=A->B, 1=A<-B, 2=both): $direction" >> $output_file
    echo "Average message interval: $lambda" >> $output_file
    echo "Trace level: $trace" >> $output_file
    echo "===============================" >> $output_file
    echo "" >> $output_file
    
    # Run the test and append results to the file
    if [ "$loss_prob" = "0.0" ] && [ "$corrupt_prob" = "0.0" ]; then
        # No direction parameter needed if no loss/corruption
        echo "$num_msgs $loss_prob $corrupt_prob $lambda $trace" | ./sr >> $output_file
    else
        # Direction parameter needed
        echo "$num_msgs $loss_prob $corrupt_prob $direction $lambda $trace" | ./sr >> $output_file
    fi
    
    echo "Test $test_name completed. Results saved to $output_file"
}

# Now run the problematic tests with trace level 1 for more details
echo -e "\n--- Running Problem Tests with More Detail ---"

# Test 3: ACK loss test (100% loss in A<-B direction)
run_test "test3_detail" 3 1.0 0.0 1 10.0 1

# Test 4: Data packet corruption test (100% corruption in A->B direction)
run_test "test4_detail" 3 0.0 1.0 0 10.0 1

# Test 5: Data packet loss test (100% loss in A->B direction)
run_test "test5_detail" 3 1.0 0.0 0 10.0 1

# Test 8: Window full test (high message rate)
run_test "test8_detail" 15 0.1 0.1 2 5.0 1

# Run the platform tests with the corrected code
echo -e "\n--- Running Platform Test Cases ---"

# Standard test cases
echo -e "\n--- Running Standard Test Cases ---"

# Test 1: Basic functionality (no loss, no corruption)
run_test "test1_basic" 3 0.0 0.0 0 10.0 0

# Test 2: Another basic test (no loss, no corruption)
run_test "test2_basic" 3 0.0 0.0 0 10.0 0

# Test 3: ACK loss test (100% loss in A<-B direction)
run_test "test3_ack_loss" 3 1.0 0.0 1 10.0 0

# Test 4: Data packet corruption test (100% corruption in A->B direction)
run_test "test4_data_corruption" 3 0.0 1.0 0 10.0 0

# Test 5: Data packet loss test (100% loss in A->B direction)
run_test "test5_data_loss" 3 1.0 0.0 0 10.0 0

# Test 6: Mixed test (moderate loss and corruption in both directions)
run_test "test6_mixed" 5 0.2 0.2 2 10.0 0

# Test 7: High load test (moderate loss and corruption)
run_test "test7_high_load" 10 0.1 0.1 2 10.0 0

# Test 8: Window full test (high message rate)
run_test "test8_window_full" 15 0.1 0.1 2 5.0 0

echo -e "\n--- All tests completed! Results are in the test_results directory. ---"
