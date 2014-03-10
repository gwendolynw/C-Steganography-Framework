
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

//returns 0 if there's not enough capacity
int embedMessageIntoCoefficients(const char *message, node *rootOfUsableCoefficientBuffer, int list_size){
    size_t message_partition_size = 1;  //let this also be known as the variable k
    size_t message_size = strlen(message)*8; //getting size of message in bits
    size_t carrier_size = list_size;
    float embed_rate = (float)message_size/carrier_size;

    while (embed_rate < ((float)message_partition_size/((1<<message_partition_size)-1))){
        message_partition_size++;
    }
    
    message_partition_size--;

    //calculate the code word length n = 2^k - 1
    int codeword_size = (1<<message_partition_size)-1;  //let this also be known as the variable n

    int doEmbed = 1;  //boolean to flag whether message partition size is practical
    
    if (message_partition_size < 2){
        doEmbed = 0;
        return 0;
    }

    //THE EMBED LOOP
    if(doEmbed){
        

        int coeff_buffer[codeword_size]; //buffer to store n bits of codeword
        int message_buffer; //buffer to store k bits of message partition
        
        int shrinkage_flag = 0; //if shrinkage, we use the message_buffer of the last iteration (because doing this with bit manipulation with weird message partitions (5 bits, etc) makes things messy
        
        int current_msgbit_index = 0; //mark how far along current bit of *message we are

        node *current_ucb_node = rootOfUsableCoefficientBuffer->next;  //mark how far along UCB we are
        
        while (*message != '\0' && current_ucb_node != NULL){
            //get size n codeword from ucb
            //i.e. fill coefficient buffer with n coefficients and store LSB of each in size n array

            for (int coeff_buffer_index = 0; coeff_buffer_index < codeword_size; coeff_buffer_index++){
                while (current_ucb_node->coeff_struct.coefficient==0) {
                    //skip zero indices, as we can't embed into them
                    remove_from_linked_list(current_ucb_node);
                    current_ucb_node = current_ucb_node->next;
                }
                coeff_buffer[coeff_buffer_index] = (current_ucb_node->coeff_struct.coefficient & 1);
                current_ucb_node = current_ucb_node->next;;
            }

            //hash coefficient buffer in order to extract k bits
            //i.e. multiply each entry in buffer by its index (starting from 1) and XOR all these products
            //as for why this words mathematically, no idea
            int hash = 0;
            for (int hash_buffer_index = 0; hash_buffer_index < codeword_size; hash_buffer_index++){
                int product = coeff_buffer[hash_buffer_index] * (hash_buffer_index+1);
                hash ^= product;
            }

            //if shrinkage is true, then the correct bits will already be the message_buffer variable
            if (!shrinkage_flag) {
                message_buffer=0;
                //fill message buffer with k bits of message string
                //this is where the bit manipulation gets cray
                for (size_t msg_buffer_index = message_partition_size; msg_buffer_index > 0; msg_buffer_index--){
                    if (current_msgbit_index > 7){  //we have moved onto the next 8-bit character, hence we must also increment pointer to get next char
                        message++;
                        current_msgbit_index = 0;
                    }

                    // we may have hit the end of the string
                    if (!*message)
                        break;

                    //get next bit of message, starting from where we last left off (relative to the current byte we're on)
                    int message_bit = (*message & (1 << (7 - current_msgbit_index))) ? 1 : 0;
                    current_msgbit_index++;
                    //store bit in this iteration's message buffer
                    if (message_bit) message_buffer |= (1 << (msg_buffer_index-1));
                    
                }
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

        } //end while (*message)
        
        if (!(*message) && current_ucb_node == NULL){
            printf("NOT ENOUGH CAPACITY\n");
            return 0;
        }
    } //end if (doEmbed)
    
    return 1;
        
}

void extractMessageFromCoefficients(node *rootOfUsableCoefficientBuffer, int list_size, size_t output_buffer_size, char *output_buffer) {
    int message_partition_size = 1;  //let this also be known as the variable k
    size_t message_length = output_buffer_size/8; //how many bytes in the message
    size_t message_index = 0; //how many bytes along have been extracted
    int message_bit_index = 0; //how many bits along (relative to current byte) have been extracted
    float embed_rate = (float)output_buffer_size/list_size;
        
    while (embed_rate < ((float)message_partition_size/((1<<message_partition_size)-1))){
        message_partition_size++;
    }
    
    message_partition_size--;

    //calculate the code word length n = 2^k - 1
    int codeword_size = (1<<message_partition_size)-1;  //let this also be known as the variable n

    node *current_ucb_node = rootOfUsableCoefficientBuffer->next; //mark how far along ucb list we are.
    
    int coeff_buffer[codeword_size]; //buffer to store n bits of codeword
    
    while (message_index < message_length){
        
        //get size n codeword from ucb
        //i.e. fill coefficient buffer with n coefficients and store LSB of each in size n array
        int coeff_buffer_index;
        for (coeff_buffer_index = 0; coeff_buffer_index < codeword_size; coeff_buffer_index++){
            while (current_ucb_node->coeff_struct.coefficient==0) {
                //skip zero indices, as we can't embed into them
                remove_from_linked_list(current_ucb_node);
                current_ucb_node = current_ucb_node->next;
            }
            coeff_buffer[coeff_buffer_index] = (current_ucb_node->coeff_struct.coefficient & 1);
            current_ucb_node = current_ucb_node->next;;
        }

        //hash buffer to extract k bits. same function as embedding.
        //i.e. multiply each entry in buffer by its index (starting from 1) and XOR all these products
        int hash = 0;
        int hash_buffer_index;
        for (hash_buffer_index = 0; hash_buffer_index < codeword_size; hash_buffer_index++){
            int product = coeff_buffer[hash_buffer_index] * (hash_buffer_index+1);
            hash ^= product;
        }

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
