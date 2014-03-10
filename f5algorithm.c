
//
//  f5methods.c
//  f5+dct
//
//  Created by Gwendolyn Weston on 7/20/13.
//  Copyright (c) 2013 Gwendolyn. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f5algorithm.h"
#include "node.h"

size_t get_message_partition_size(size_t message_size_in_bits, size_t list_size) {
    size_t message_partition_size = 1;
    float embed_rate = (float)message_size_in_bits/list_size;

    while (embed_rate < ((float)message_partition_size/((1<<message_partition_size)-1))){
        message_partition_size++;
    }
    message_partition_size--;

    return message_partition_size;
}

void fill_coeff_buffer_with_n_coefficients_lsb (int *coeff_buffer, node *coeff_node, size_t n) {
    for (int i = 0; i < n; i++){
        while (coeff_node->coeff_struct.coefficient==0) {
            remove_from_linked_list(coeff_node);
            coeff_node = coeff_node->next;
        }
        coeff_buffer[i] = (coeff_node->coeff_struct.coefficient & 1);
        coeff_node = coeff_node->next;
    }
}

//guys guys guys, the math behind this part is so fucking cool
//i wish the implementation didn't obscure it so much
//but basically, multiply each entry in buffer by its index (starting from 1) and XOR all these products
int hash_coefficient_buffer(int *coeff_buffer, size_t n) {
    int hash = 0;
    for (int i = 0; i < n; i++){
        int product = coeff_buffer[i] * (i+1);
        hash ^= product;
    }
    return hash;
}

int fill_message_buffer_with_k_message_bits (int *message_buffer, const char *message, size_t k, int *current_msgbit_index) {
    message_buffer=0;
    for (size_t i = k; i > 0; i--){
        if (*current_msgbit_index > 7){
            message++;
            *current_msgbit_index = 0;
        }

        // we may have hit the end of the string
        if (!*message)
            return 1;

        //get next bit of message, starting from where we last left off (relative to the current byte we're on)
        int message_bit = (*message & (1 << (7 - *current_msgbit_index))) ? 1 : 0;
        current_msgbit_index++;
        //store bit in this iteration's message buffer
        if (message_bit) *message_buffer |= (1 << (i-1));
    }

    return 0;
}
}


int embedMessageIntoCoefficients(const char *message, node *rootOfUsableCoefficientBuffer, int list_size){
    size_t message_partition_size = get_message_partition_size(strlen(message)*8, list_size); //k
    size_t codeword_size = (1<<message_partition_size)-1; //n

    if (message_partition_size < 2){
        return -1;
    }
    else {
        int coeff_buffer[codeword_size];
        int message_buffer;
        int shrinkage_flag = 0;
        int current_msgbit_index = 0;
        int end_of_message = 0;

        node *current_ucb_node = rootOfUsableCoefficientBuffer->next;
        
        while (*message != '\0' && current_ucb_node != NULL){

            fill_coeff_buffer_with_n_coefficients_lsb(coeff_buffer, current_ucb_node, codeword_size);
            int hash = hash_coefficient_buffer(coeff_buffer, codeword_size);

            //if shrinkage is true, then the correct bits will already be the message_buffer variable
            if (!shrinkage_flag) {
                message_buffer=0;
                end_of_message = fill_message_buffer_with_k_message_bits(&message_buffer, message, message_partition_size, &current_msgbit_index);
            }
            int index_to_change = (message_buffer ^ hash);

            //decrement/increment absolute value of index to change
            if (!index_to_change) {
                continue; //if zero, don't do any embedding
            }
            
            //embed the secret message: change message at index of sum
            //because we only know how far along we are in the ucb array in relative to codeword_size
            //as in, we need to figure out the index to change relative to the increment
            //also, the array is zero-indexed, so we have to subtract one for that
            
            node *node_to_change = traverse_n_nodes_backward(current_ucb_node, codeword_size);
            node_to_change = traverse_n_nodes_forward(node_to_change, index_to_change-1);

            buffer_coefficient *coeff = &node_to_change->coeff_struct;
            if (coeff->coefficient > 0)
                coeff->coefficient--;
            else
                coeff->coefficient++;

            //embed the secret message: if shrinkage, repeat iteration. because of condition while filling coefficient buffer, zero will be ignored
            if (coeff->coefficient == 0){
                current_ucb_node = traverse_n_nodes_backward(current_ucb_node, codeword_size);
                shrinkage_flag = 1;
            }
            else {
                shrinkage_flag = 0;
            }
            if (end_of_message)
                break;

        } //end while (*message)
        
        if (!(*message) && current_ucb_node == NULL){
            printf("NOT ENOUGH CAPACITY\n");
            return 0;
        }
    } //end if (doEmbed)
    
    return 1;
        
}

void extractMessageFromCoefficients(node *rootOfUsableCoefficientBuffer, int list_size, size_t output_buffer_size, char *output_buffer) {
    size_t message_partition_size = get_message_partition_size(output_buffer_size, list_size);
    size_t message_size_in_bytes = output_buffer_size/8;
    size_t message_index = 0; //how many bytes along have been extracted
    int message_bit_index = 0; //how many bits along (relative to current byte) have been extracted

    //calculate the code word length n = 2^k - 1
    int codeword_size = (1<<message_partition_size)-1;  //let this also be known as the variable n
    node *current_ucb_node = rootOfUsableCoefficientBuffer->next; //mark how far along ucb list we are.
    int coeff_buffer[codeword_size]; //buffer to store n bits of codeword
    
    while (message_index < message_size_in_bytes){
        
        fill_coeff_buffer_with_n_coefficients_lsb(coeff_buffer, current_ucb_node, codeword_size);
        int hash = hash_coefficient_buffer(coeff_buffer, codeword_size);

        //add each bit from k-size hash, starting from the left into current index of extracted message array
        int current_hash_bit;
        for (current_hash_bit = 0; current_hash_bit < message_partition_size; current_hash_bit++){
            if (message_bit_index > 7){ //we reach end of byte, so increment to next byte of allocated array
                message_bit_index = 0;
                message_index++;
                output_buffer[message_index] = 0;
            }
            
            //get left-most bit from hash
            int message_bit = (hash & 1 << (message_partition_size - current_hash_bit -1)) ? 1 : 0;

            //add bit to message's current byte
            if (message_bit) output_buffer[message_index] |= (1 << (7-message_bit_index));
            message_bit_index++;
            
        }

    } //end while (message_index < message_length)

}// end extraction
