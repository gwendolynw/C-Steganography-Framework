//
//  main.c
//  f5
//
//  Created by Gwendolyn Weston on 7/20/13.
//  Copyright (c) 2013 Gwendolyn. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct buffer_coefficient buffer_coefficient;

//struct containing coefficient and metadata
struct buffer_coefficient {
    int row_index;
    int column_index;
    int block_index;
    int coefficient;
};

typedef struct node node;

//linked list node
struct node{
    buffer_coefficient coeff_struct;
    node *next;
    node *prev;
};


void add_to_linked_list(node *new_node, node *current_node) {
    new_node->prev = current_node;
    new_node->next = current_node->next;
    current_node->next = new_node;
}

void remove_from_linked_list(node *delete_node){
    delete_node->next->prev = delete_node->prev;
    delete_node->prev->next = delete_node->next;
    
}

node *traverse_n_nodes_forward(node *current_node, int n) {
    int index;
    for (index = 0; index<n; index++) {
        current_node = current_node->next;
    }
    return current_node;
}

node *traverse_n_nodes_backward(node *current_node, int n) {
    int index;
    for (index = 0; index<n; index++) {
        current_node = current_node->prev;
    }
    return current_node;
}

void print_linked_list(node *root) {
    node *current_node = root->next;
    
    while(current_node != NULL) {
        printf("%i ", current_node->coeff_struct.coefficient);
        current_node = current_node->next;
    }
    
}

//returns 0 if there's not enough capacity
int embedMessageIntoCoefficients(char *message, node *rootOfUsableCoefficientBuffer, int list_size){
    int message_partition_size = 1;  //let this also be known as the variable k
    int message_size = (int)strlen(message)*8; //getting size of message in bits
    int carrier_size = list_size;
    float embed_rate = (float)message_size/carrier_size;
    
    printf("message_size is %i and carrier_size is %i\n", message_size, carrier_size);
    
    while (embed_rate < ((float)message_partition_size/((1<<message_partition_size)-1))){
        message_partition_size++;
    }
    
    message_partition_size--;
    printf("message will be partitioned into %i bits\n", message_partition_size);
    
    //calculate the code word length n = 2^k - 1
    int codeword_size = (1<<message_partition_size)-1;  //let this also be known as the variable n
    
    printf("carrier will be partitioned into %i codewords\n", codeword_size);
    
    printf("\n");
    
    int doEmbed = 1;  //boolean to flag whether message partition size is practical
    
    if (message_partition_size < 2){
        doEmbed = 0;
        return 0;
    }
    
    //THE EMBED LOOP
    if(doEmbed){
        
        int coeff_buffer[codeword_size]; //buffer to store n bits of codeword
        int message_buffer; //buffer to store k bits of message partition
        
        int current_msgbit_index = 0; //mark how far along current bit of *message we are
        
        node *current_ucb_node = rootOfUsableCoefficientBuffer->next;  //mark how far along UCB we are
        
        while (*message && current_ucb_node != NULL){
            //get size n codeword from ucb
            //i.e. fill coefficient buffer with n coefficients and store LSB of each in size n array
            int coeff_buffer_index;
            //printf("this iteration's coefficient buffer is: ");
            for (coeff_buffer_index = 0; coeff_buffer_index < codeword_size; coeff_buffer_index++){
                while (current_ucb_node->coeff_struct.coefficient==0) {
                    //skip zero indices, as we can't embed into them
                    remove_from_linked_list(current_ucb_node);
                    current_ucb_node = current_ucb_node->next;
                }
                printf("%i ", current_ucb_node->coeff_struct.coefficient);
                coeff_buffer[coeff_buffer_index] = (current_ucb_node->coeff_struct.coefficient & 1);
                current_ucb_node = current_ucb_node->next;;
            }
            
            //printf("\n");
            
            
            //DEBUG PRINTING HERE
            //printf("this iteration's coefficient buffer LSB is: ");
            for (coeff_buffer_index = 0; coeff_buffer_index < codeword_size; coeff_buffer_index++){
                //printf("%i", coeff_buffer[coeff_buffer_index]);
            }
            //DEBUG PRINTING END
            
            //printf("\n");
            
            //hash coefficient buffer in order to extract k bits
            //i.e. multiply each entry in buffer by its index (starting from 1) and XOR all these products
            //as for why this words mathematically, no idea
            int hash = 0;
            int hash_buffer_index;
            for (hash_buffer_index = 0; hash_buffer_index < codeword_size; hash_buffer_index++){
                int product = coeff_buffer[hash_buffer_index] * (hash_buffer_index+1);
                hash ^= product;
            }
            
            //fill message buffer with k bits of message string
            //this is where the bit manipulation gets cray
            int msg_buffer_index = 0;
            message_buffer=0;
            
            //printf("message bit is: ");
            for (msg_buffer_index = message_partition_size; msg_buffer_index > 0; msg_buffer_index--){
                if (current_msgbit_index > 7){  //we have moved onto the next 8-bit character, hence we must also increment pointer to get next char
                    *message++;
                    current_msgbit_index = 0;
                }
                
                //get next bit of message, starting from where we last left off (relative to the current byte we're on)
                int message_bit = (*message & 1 << (7 - current_msgbit_index)) ? 1 : 0;
                current_msgbit_index++;
                //printf("%i", message_bit);
                
                //store bit in this iteration's message buffer
                if (message_bit) message_buffer |= (1 << (msg_buffer_index-1));
                
            }
            
            //printf("\n");
            
            //printf("this iteration's message buffer is: %i\n", message_buffer);
            int index_to_change = (message_buffer ^ hash);
            
            //printf("index to change is: %i\n", index_to_change);
            
            //decrement/increment absolute value of index to change
            if (!index_to_change) {
                //printf("\n");
                continue; //if zero, don't do any embedding
            }
            
            //embed the secret message: change message at index of sum
            //because we only know how far along we are in the ucb array in relative to codeword_size
            //as in, we need to figure out the index to change relative to the increment
            //also, the array is zero-indexed, so we have to subtract one for that
            
            node *node_to_change = traverse_n_nodes_backward(current_ucb_node, codeword_size);
            //printf("1: coefficient to change is: %i\n", node_to_change->coeff_struct.coefficient);
            
            node_to_change = traverse_n_nodes_forward(node_to_change, index_to_change-1);
            
            //printf("2: coefficient to change is: %i\n", node_to_change->coeff_struct.coefficient);
            
            (node_to_change->coeff_struct.coefficient>0)?node_to_change->coeff_struct.coefficient--: node_to_change->coeff_struct.coefficient++;
            
            //printf("coefficient changed to: %i\n", node_to_change->coeff_struct.coefficient);
            
            //DEBUG PRINTING HERE
            //printf("this iteration's changed coefficient buffer is: ");
            node *debug_node = traverse_n_nodes_backward(current_ucb_node, codeword_size);
            int debug_index;
            for (debug_index = 0; debug_index < codeword_size; debug_index++){
                //printf(" %i", debug_node->coeff_struct.coefficient);
                debug_node = debug_node->next;
            }
            //DEBUG PRINTING END
            //printf("\n");
            
            
            //embed the secret message: if shrinkage, repeat iteration. because of condition while filling coefficient buffer, zero will be ignored
            if (node_to_change->coeff_struct.coefficient == 0){
                current_ucb_node = traverse_n_nodes_backward(current_ucb_node, codeword_size);
                current_msgbit_index -= message_partition_size;
                //printf("SHRINKAGE\n");
            }
            //printf("\n");
            
            
        } //end while (*message)
        
        if (!(*message) && current_ucb_node == NULL){
            printf("NOT ENOUGH CAPACITY\n");
            return 0;
        }
    } //end if (doEmbed)
    
    return 1;
    
}

char *extractMessageFromCoefficients(node *rootOfUsableCoefficientBuffer, int list_size, int message_size, char *extracted_message_string) {
    int message_partition_size = 1;  //let this also be known as the variable k
    int message_length = message_size/8; //how many bytes in the message
    int message_index = 0; //how many bytes along have been extracted
    int message_bit_index = 0; //how many bits along (relative to current byte) have been extracted
    float embed_rate = (float)message_size/list_size;
    
    //printf("message_size is %i and carrier_size is %i\n", message_size, list_size);
    
    while (embed_rate < ((float)message_partition_size/((1<<message_partition_size)-1))){
        message_partition_size++;
    }
    
    message_partition_size--;
    //printf("message will be partitioned into %i bits\n", message_partition_size);
    
    //calculate the code word length n = 2^k - 1
    int codeword_size = (1<<message_partition_size)-1;  //let this also be known as the variable n
    
    //printf("carrier will be partitioned into %i codewords\n", codeword_size);
    
    printf("\n");
    
    int *extracted_message = malloc(sizeof(int)*message_length); //create array to store extracted message
    //extracted_message[message_index] = 0;
    
    node *current_ucb_node = rootOfUsableCoefficientBuffer->next; //mark how far along ucb list we are.
    
    int coeff_buffer[codeword_size]; //buffer to store n bits of codeword
    
    while (message_index < message_length){
        
        //get size n codeword from ucb
        //i.e. fill coefficient buffer with n coefficients and store LSB of each in size n array
        int coeff_buffer_index;
        printf("coefficient buffer is: ");
        for (coeff_buffer_index = 0; coeff_buffer_index < codeword_size; coeff_buffer_index++){
            while (current_ucb_node->coeff_struct.coefficient==0) {
                //skip zero indices, as we can't embed into them
                remove_from_linked_list(current_ucb_node);
                current_ucb_node = current_ucb_node->next;
            }
            printf(" %i", current_ucb_node->coeff_struct.coefficient);
            coeff_buffer[coeff_buffer_index] = (current_ucb_node->coeff_struct.coefficient & 1);
            current_ucb_node = current_ucb_node->next;;
        }
        
        printf("\n");
        
        //hash buffer to extract k bits. same function as embedding.
        //i.e. multiply each entry in buffer by its index (starting from 1) and XOR all these products
        int hash = 0;
        int hash_buffer_index;
        for (hash_buffer_index = 0; hash_buffer_index < codeword_size; hash_buffer_index++){
            int product = coeff_buffer[hash_buffer_index] * (hash_buffer_index+1);
            hash ^= product;
        }
                
        //printf("hash is: %i\n", hash);
        
        //add each bit from k-size hash, starting from the left into current index of extracted message array
        int current_hash_bit;
        printf("message bit is: ");
        for (current_hash_bit = 0; current_hash_bit < message_partition_size; current_hash_bit++){
            if (message_bit_index > 7){ //we reach end of byte, so increment to next byte of allocated array
                message_bit_index = 0;
                printf(" (here is newly minted number: %i) ", extracted_message[message_index]);
                message_index++;
                extracted_message[message_index] = 0;
            }
            
            //get left-most bit from hash
            int message_bit = (hash & 1 << (message_partition_size - current_hash_bit -1)) ? 1 : 0;
            printf("%i", message_bit);
            
            //add bit to message's current byte
            if (message_bit) extracted_message[message_index] |= (1 << (7-message_bit_index));
            message_bit_index++;
            
        }
        printf("\n\n");

        
    } //end while (message_index < message_length)
    

    
    int b;
    //printf("\nHere is message: {");
    for (b = 0; b<message_length; b++){
        sprintf(&extracted_message_string[b], "%c", (char)extracted_message[b]);
        //printf("%c, ", (char)extracted_message[b]);
    }
    //printf("}\n");
    
    
    int c;
    printf("\n\nHere is message (as represented in bytes): {");
    for (c = 0; c<message_length; c++){
        printf("%i, ", extracted_message[c]);
    }
    printf("}\n");
     
    

    printf("\nextracted string is: \n%s\n", extracted_message_string);
    
    return extracted_message_string;
    
}// end extraction


int main(int argc, const char * argv[])
{
    //first, we create the linked list header
    node *root = malloc(sizeof(node));
    node *end = malloc(sizeof(node));
    
    root->next = end;
    end->prev = root;
    end->next = NULL;
    node *current_node = root;
    
    int index;
    int list_size = 100;
    
    //then make linked list of 350
    //this will be our usable coefficient buffer
    for (index = 0; index<list_size; index++){
        int random_number = arc4random() % 150;
        random_number =  (random_number%2)? random_number*-1: random_number;
        
        node *new_node = malloc(sizeof(node));
        new_node->coeff_struct.coefficient = random_number;
        
        add_to_linked_list(new_node, current_node);
        current_node = current_node->next;
    }
    
    printf("\n");
    printf("loop coefficients are: ");
    //print_linked_list(root);
    printf("\n");
    
    char *message = "Hello";/* that whirl me I know not whither\nYour schemes, politics, fail, lines give way, substances mock and elude me,\nOnly the theme I sing, the great and strong-possess'd soul, eludes not,\nOne's-self must never give way--that is the final substance--that\nout of all is sure,\nOut of politics, triumphs, battles, life, what at last finally remains?\nWhen shows break up what but One's-Self is sure?";*/
    
    embedMessageIntoCoefficients(message, root, list_size);
    
    printf("\n\nour stego coefficients: \n");
    print_linked_list(root);
    
    printf("\n");
    int message_size = (int)strlen(message)*8; //getting size of message in bits
    char *extractedMessage = malloc(sizeof(char) *message_size);
    extractMessageFromCoefficients(root, list_size, message_size, extractedMessage);
    
    return 0;
}

