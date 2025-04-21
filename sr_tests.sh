#!/bin/bash

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

# Run all tests
echo "Starting SR Protocol tests..."

# Test 1: Basic functionality (no loss, no corruption)
run_test "test1_basic" 5 0.0 0.0 0 10.0 2


