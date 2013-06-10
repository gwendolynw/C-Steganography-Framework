#include <stdio.h>
#include <string.h>


int main(int argc, const char * argv[])
{
    int UCB[320]  ={ 19, -29, -13, -28, 47, 38, -30, -19, 14, 33, 18, 36, 3, -16, -41, 26, -41, 44, -3, -44, 15, 2, -30, 9, 34, -15, -3, 49, -25, -38, 9, -14, 34, -1, 24, 40, 4, 3, 10, 17, 0, -14, 29, -40, 27, 34, 43, 3, -11, -34, 48, -12, -19, -5, -28, 6, 28, -9, -25, -34, -34, 22, 43, 29, 46, 42, 40, 37, 21, 29, 24, 14, 25, 25, -46, 48, -46, -39, -33, 33, 3, 47, 34, 23, 48, -22, -10, 22, -6, -42, -19, 21, -37, -14, 15, 9, 37, -38, 34, -8, -24, 44, 33, 8, 1, -41, 0, 8, -21, 41, 21, -46, -37, -22, -20, -23, -46, -18, 30, 18, -27, 38, -21, 34, 13, -18, 48, -49, 14, -13, 33, -21, -46, 11, -26, 18, -7, -37, -41, -27, 24, 18, 7, 0, -44, -1, 22, 35, 33, 33, 19, -29, -13, -28, 47, 38, -30, -19, 14, 33, 18, 36, 3, -16, -41, 26, -41, 44, -3, -44, 15, 2, -30, 9, 34, -15, -3, 49, -25, -38, 9, -14, 34, -1, 24, 40, 4, 3, 10, 17, 0, -14, 29, -40, 27, 34, 43, 3, -11, -34, 48, -12, -19, -5, -28, 6, 28, -9, -25, -34, -34, 22, 43, 29, 46, 42, 40, 37, 21, 29, 24, 14, 25, 25, -46, 48, -46, -39, -33, 33, 3, 47, 34, 23, 48, -22, -10, 22, -6, -42, -19, 21, -37, -14, 15, 9, 37, -38, 34, -8, -24, 44, 33, 8, 1, -41, 0, 8, -21, 41, 21, -46, -37, -22, -20, -23, -46, -18, 30, 18, -27, 38, -21, 34, 13, -18, 48, -49, 14, -13, 33, -21, -46, 11, -26, 18, -7, -37, -41, -27, 24, 18, 7, 0, -44, -1, 22, 35, 33, 33, 41, -27, 24, 18, 7, 0, -44, -1, 22, 35, 33, 33, 22, 35, 33, 33, 22, 35, 33, 33};  //Usable Coefficient Buffer from DCT
        char *message = "Hello asdjklf";
        
        int message_partition_size = 1;  //let this also be known as the variable k
        int message_size = strlen(message)*8; //getting size of message in bits
        int carrier_size = 320;
        float embed_rate = (float)message_size/carrier_size;
        
        printf("determined message_size is %i and carrier_size is %i\n", message_size, carrier_size);
        
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
        int doExtract = 1; //boolean to flag whether capacity has encoded message
        
        if (message_partition_size < 2){
            doEmbed = 0;
        }
        
        //THE EMBED LOOP
        if(doEmbed){
            int coeff_buffer[codeword_size]; //buffer to store n bits of codeword
            int message_buffer; //buffer to store k bits of message partition
            
            int current_ucb_index = 0; //mark how far along ucb array we are
            int current_msgbit_index = 0; //mark how far along current bit of *message we are
            
            
            while (*message && current_ucb_index <= carrier_size){
                
                int hasZeroes = 0; //keep track of how many zeroes were skipped in UCB (this is so that we change the right coefficient)
                
                //get size n codeword from ucb
                //i.e. fill coefficient buffer with n coefficients and store LSB of each in size n array
                int coeff_buffer_index;
                printf("this iteration's coefficient buffer is: ");
                for (coeff_buffer_index = 0; coeff_buffer_index < codeword_size; coeff_buffer_index++){
                    while (UCB[current_ucb_index]==0) {
                        current_ucb_index++;  //skip zero indices, as we can't embed into them
                        hasZeroes++;
                    }
                    printf(" %i", UCB[current_ucb_index]);
                    coeff_buffer[coeff_buffer_index] = (UCB[current_ucb_index] & 1);
                    current_ucb_index++;
                }
                
                printf("\n");

                
                //DEBUG PRINTING HERE
                printf("this iteration's coefficient buffer LSB is: ");
                for (coeff_buffer_index = 0; coeff_buffer_index < codeword_size; coeff_buffer_index++){
                    printf("%i", coeff_buffer[coeff_buffer_index]);
                }
                //DEBUG PRINTING END

                
                printf("\n");
                
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
                
                printf("message bit is: ");
                for (msg_buffer_index = message_partition_size; msg_buffer_index > 0; msg_buffer_index--){
                    if (current_msgbit_index > 7){  //we have moved onto the next 8-bit character, hence we must also increment pointer to get next char
                        *message++;
                        current_msgbit_index = 0;
                    }
                    
                    //get next bit of message, starting from where we last left off (relative to the current byte we're on)
                    int message_bit = (*message & 1 << (7 - current_msgbit_index)) ? 1 : 0;
                    current_msgbit_index++;
                    printf("%i", message_bit);
                    
                    //store bit in this iteration's message buffer
                    if (message_bit) message_buffer |= (1 << (msg_buffer_index-1));
                    
                }
                
                printf("\n");
                
                //XOR k bits of the message against hash
                //Consider this the matrix encoding part of the algorithm: a XOR b
                //where, a XOR b XOR b = a  ==> message_bits XOR codeword XOR codeword = message_bits
                
                printf("this iteration's message buffer is: %i\n", message_buffer);
                int index_to_change = (message_buffer ^ hash);
                
                printf("index to change is: %i\n", index_to_change);
                
                //decrement/increment absolute value of index to change
                if (!index_to_change) {
                    printf("\n");
                    continue; //if zero, don't do any embedding
                }
                
                //embed the secret message: change message at index of sum
                //because we only know how far along we are in the ucb array in relative to codeword_size 
                //as in, we need to figure out the index to change relative to the increment
                //because we might have skipped zeroes when filling the buffer, we also need to subtract that along with codeword_size
                //also, the array is zero-indexed, so we have to subtract one for that
                
                index_to_change = current_ucb_index-codeword_size - hasZeroes + index_to_change -1;
                printf("there have been %i zeroes\n", hasZeroes);
                
                
                //also, we need to check if there are any zeros in ucb before index_to_change which will throw it off
                //i'm starting to think this might have been better as a linked list
                int checkingForZero;
                for (checkingForZero = current_ucb_index-codeword_size - hasZeroes; checkingForZero <= index_to_change; checkingForZero++){
                    if (UCB[checkingForZero] == 0) index_to_change++;
                }
                
                printf("coefficient to change is: %i\n", UCB[index_to_change]);

                (UCB[index_to_change]>0)?UCB[index_to_change]--: UCB[index_to_change]++;
                
                printf("coefficient changed to: %i\n", UCB[index_to_change]);
                
                //DEBUG PRINTING HERE
                printf("this iteration's changed coefficient buffer is: ");
                int debug_index;
                for (debug_index = (current_ucb_index - codeword_size - hasZeroes); debug_index < current_ucb_index+hasZeroes; debug_index++){
                    printf(" %i", UCB[debug_index]);
                }
                //DEBUG PRINTING END
                
                printf("\n");

                
                //embed the secret message: if shrinkage, repeat iteration. because of condition while filling coefficient buffer, zero will be ignored
                if (UCB[index_to_change] == 0){
                    current_ucb_index -= codeword_size;
                    current_msgbit_index -= message_partition_size;
                    printf("SHRINKAGE\n");
                }
                printf("\n");

                
            } //end while (*message)
            
            if (current_ucb_index > carrier_size){
                printf("NOT ENOUGH CAPACITY\n");
                doExtract = (current_ucb_index > carrier_size)?0:1;
            }
            
        } //end if (doEmbed)
        
        int arrayIndex;
        printf("\nHere is stego UCB: {");
        for (arrayIndex = 0; arrayIndex<carrier_size; arrayIndex++){
            printf("%i, ", UCB[arrayIndex]);
        }
        printf("}\n");
        
        //THE EXTRACT LOOP
        //Let's assume that we're somehow given the message size and from there, we get n and k again
        //maybe prepend password
        
        
        if (doExtract){
            
            int message_length = message_size/8; //how many bytes in the message
            int message_index = 0; //how many bytes along have been extracted
            int message_bit_index = 0; //how many bits along (relative to current byte) have been extracted
            int extracted_message[message_length];
            
            extracted_message[message_index] = 0;
            
            //VARIABLES RE-USED FROM EXTRACTION LOOP, FYI.
            int current_ucb_index = 0; //mark how far along ucb array we are.
            int coeff_buffer[codeword_size]; //buffer to store n bits of codeword
            
            while (message_index < message_length){
                
                //fill buffer with n nonzero coefficients
                printf("this iteration's coefficient buffer is: ");
                int coeff_buffer_index;
                for (coeff_buffer_index = 0; coeff_buffer_index < codeword_size; coeff_buffer_index++){
                    while (UCB[current_ucb_index]==0) current_ucb_index++;  //skip zero indices, as we can't extract from them
                    coeff_buffer[coeff_buffer_index] = (UCB[current_ucb_index] & 1);
                    printf(" %i", UCB[current_ucb_index]);
                    current_ucb_index++;
                }
                
                //hash buffer to extract k bits. same function as embedding.
                //i.e. multiply each entry in buffer by its index (starting from 1) and XOR all these products
                int hash = 0;
                int hash_buffer_index;
                for (hash_buffer_index = 0; hash_buffer_index < codeword_size; hash_buffer_index++){
                    int product = coeff_buffer[hash_buffer_index] * (hash_buffer_index+1);
                    hash ^= product;
                }
                printf("\n");
                
                printf("hash is: %i\n", hash);
                
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
                

                
            } //end while (message_index < message_length)
            
            int b;
            printf("\nHere is message: {");
            for (b = 0; b<message_length; b++){
                printf("%c, ", (char)extracted_message[b]);
            }
            printf("}\n");
            
            int c;
            printf("\nHere is int message: {");
            for (c = 0; c<message_length; c++){
                printf("%i, ", extracted_message[c]);
            }
            printf("}\n");
            
        }// end if (doExtract)
        
        else printf("CAN'T TEST EXTRACTION, MESSAGE WASN'T ENCODED COMPLETELY");
        
}
