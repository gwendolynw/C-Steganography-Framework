/*
 * dct.c
 *
 * Copyright (C) 2012, Owen Campbell-Moore.
 *
 * This program allows you to take JPEG, modify it's DCT coefficients and
 * then output the new coefficients to a new JPEG.
 *
 */

#include "cdjpeg.h"   /* Common decls for compressing and decompressing jpegs */
#include "cprngssl.c"
#include <string.h>



LOCAL(void)
usage (void)
/* complain about bad command line */
{
  fprintf(stderr, "\nusage: dct inputfile outputfile\n\n");
  exit(EXIT_FAILURE);
}

/*
 * The main program.
 */

int
main (int argc, char **argv)
{
  struct jpeg_decompress_struct inputinfo;
  struct jpeg_compress_struct outputinfo;
  struct jpeg_error_mgr jerr;
  jvirt_barray_ptr *coef_arrays;
  JDIMENSION i, compnum, rownum, blocknum;
  JBLOCKARRAY coef_buffers[MAX_COMPONENTS];
  JBLOCKARRAY row_ptrs[MAX_COMPONENTS];
  FILE * input_file;
  FILE * output_file;

  /* Handle arguments */
  if (argc != 3) usage();
  char *inputname;
  inputname = argv[1];
  char *outputname;
  outputname = argv[2];

  /* Open the input and output files */
  if ((input_file = fopen(inputname, READ_BINARY)) == NULL) {
      fprintf(stderr, "Can't open %s\n", inputname);
      exit(EXIT_FAILURE);
  }
  if ((output_file = fopen(outputname, WRITE_BINARY)) == NULL) {
    fprintf(stderr, "Can't open %s\n", outputname);
    exit(EXIT_FAILURE);
  }

  /* Initialize the JPEG compression and decompression objects with default error handling. */
  inputinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&inputinfo);
  outputinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&outputinfo);

  /* Specify data source for decompression and recompression */
  jpeg_stdio_src(&inputinfo, input_file);
  jpeg_stdio_dest(&outputinfo, output_file);

  /* Read file header */
  (void) jpeg_read_header(&inputinfo, TRUE);

  /* Allocate memory for reading out DCT coeffs */
  for (compnum=0; compnum<inputinfo.num_components; compnum++)
    coef_buffers[compnum] = ((&inputinfo)->mem->alloc_barray) 
                            ((j_common_ptr) &inputinfo, JPOOL_IMAGE,
                             inputinfo.comp_info[compnum].width_in_blocks,
                             inputinfo.comp_info[compnum].height_in_blocks);

  /* Read input file as DCT coeffs */
  coef_arrays = jpeg_read_coefficients(&inputinfo);

  /* Copy compression parameters from the input file to the output file */
  jpeg_copy_critical_parameters(&inputinfo, &outputinfo);

  /* Copy DCT coeffs to a new array */
  int num_components = inputinfo.num_components;
  size_t block_row_size[num_components];
  int width_in_blocks[num_components];
  int height_in_blocks[num_components];
  for (compnum=0; compnum<num_components; compnum++)
  {
    height_in_blocks[compnum] = inputinfo.comp_info[compnum].height_in_blocks;
    width_in_blocks[compnum] = inputinfo.comp_info[compnum].width_in_blocks;
    block_row_size[compnum] = (size_t) SIZEOF(JCOEF)*DCTSIZE2*width_in_blocks[compnum];
    for (rownum=0; rownum<height_in_blocks[compnum]; rownum++)
    {
      row_ptrs[compnum] = ((&inputinfo)->mem->access_virt_barray) 
                          ((j_common_ptr) &inputinfo, coef_arrays[compnum], 
                            rownum, (JDIMENSION) 1, FALSE);
      for (blocknum=0; blocknum<width_in_blocks[compnum]; blocknum++)
      {
        for (i=0; i<DCTSIZE2; i++)
        {
          coef_buffers[compnum][rownum][blocknum][i] = row_ptrs[compnum][0][blocknum][i];
        }
      }
    }
  }

  //BEFORE MODIFICATION
 /* Print out DCT coefficients */
  for (compnum=0; compnum<num_components; compnum++)
  {
    for (rownum=0; rownum<height_in_blocks[compnum]; rownum++)
    {
      for (blocknum=0; blocknum<width_in_blocks[compnum]; blocknum++)
      {
        printf("\n\nComponent: %i, Row:%i, Column: %i\n", compnum, rownum, blocknum);
        for (i=0; i<DCTSIZE2; i++)
        {
          coef_buffers[compnum][rownum][blocknum][i] = - coef_buffers[compnum][rownum][blocknum][i];
          printf("%i,", coef_buffers[compnum][rownum][blocknum][i]);
        }
      }
    }
  }


    //PER WESTFIELD ALGORITHM IN PAPER:
    //initialise a cryptographically strong random number generator with a key derived from the password

    char *password = "password"; //ADD WAY TO READ IN PASSWORD LATER.
    char *message = "message"; //ADD WAY TO READ IN MESSAGE LATER.
    RAND_seed(password, strlen(password));  //HOW TO GET NUMBER OF BITS IN MESSAGE? IS STRLEN RIGHT?

    //instantiate a permutation (two parameters: random generator and number of coefficients)
    int permutationArray[num_components];
    int index;
    
    for (index = 0; index<num_components; index++){
      permutationArray[index]=index;
    }

    int randomIndex, temp;
    int maxRandom = num_components;
    for (index = 0; index<num_components; index++){
        randomIndex = spc_rand_range(0, maxRandom--);
        temp = permutationArray[randomIndex];
        permutationArray[randomIndex] = permutationArray[maxRandom];
        permutationArray[maxRandom] = temp;
      }

    //determine the parameter k from the capacity of the carrier medium and the length of the secret message
    int k = 1;
    int message_size = strlen(message); 
    int carrier_size = num_components; //IS CARRIER SIZE IS NUMBER OF COEFFICIENTS? OR NUMBER OF BITS IN JPEG?
    float embed_rate = (float)message_size/carrier_size;

    while (embed_rate < ((float)k/((1<<k)-1))){
      k++;
    }

    k--;
    
    //calculate the code wode lenght n = 2^k - 1
    int n = (1<<k)-1;

    int shuffledCoeffs[n];
    int messageBuffer[k];
    int currentCoeff = 0;
    int currentMessageBit = 0;
    int hash = 0;

    while (currentMessageBit < message_size){
    //embed the secret message: fill buffer with n nonzero coefficients. order is determined by above permutation array
      int i;
      for (i = 0; i < n; i++){
        shuffledCoeffs[i] = (intptr_t)(coef_buffers[permutationArray[i]]);
        //embed the secret message: hash this buffer (i.e. multiply LSB of entry by its index and then xor).  
        hash ^= (shuffledCoeffs[i] & 1) * i; //THIS DOESN'T SEEM RIGHT, BUT I'M NOT SURE HOW ELSE TO CODE FORMULA IN PAPER.  (Do I need to convert index to binary before multiplying? Would I need to also implement binary multiplcation?)
      }

      int j;  //the next k bits of the message
      for (j = 0; j < k; i++){
        messageBuffer[j] = message[j]; 
      }

    //embed the secret message: add next k bits to hash value (xor bit by bit)
    int m;
    for (m = 0; m < k ; m++){
        hash ^= messageBuffer[m]; //THIS ALSO DOESN'T SEEM RIGHT.  How to XOR bit by bit, rather than by each char in the message?
    }

    int indexToChange = hash;

    if (!indexToChange) continue; 

    //embed the secret message: change message at index of sum
    (coef_buffers[indexToChange]>0)?coef_buffers[indexToChange]--: coef_buffers[indexToChange]++;

    //embed the secret message: if shrinkage, repeat steps again
    if (coef_buffers[indexToChange] == 0){
      currentCoeff -= n;
      currentMessageBit -= k;
    }

    currentCoeff += n;
    currentMessageBit += k;
    }

    //AFTER MODIFICATION
   /* Print out DCT coefficients */
  for (compnum=0; compnum<num_components; compnum++)
  {
    for (rownum=0; rownum<height_in_blocks[compnum]; rownum++)
    {
      for (blocknum=0; blocknum<width_in_blocks[compnum]; blocknum++)
      {
        printf("\n\nComponent: %i, Row:%i, Column: %i\n", compnum, rownum, blocknum);
        for (i=0; i<DCTSIZE2; i++)
        {
          coef_buffers[compnum][rownum][blocknum][i] = - coef_buffers[compnum][rownum][blocknum][i];
          printf("%i,", coef_buffers[compnum][rownum][blocknum][i]);
        }
      }
    }
  }

  /* Output the new DCT coeffs to a JPEG file */
  for (compnum=0; compnum<num_components; compnum++)
  {
    for (rownum=0; rownum<height_in_blocks[compnum]; rownum++)
    {
      row_ptrs[compnum] = ((&outputinfo)->mem->access_virt_barray) 
                          ((j_common_ptr) &outputinfo, coef_arrays[compnum], 
                           rownum, (JDIMENSION) 1, TRUE);
      memcpy(row_ptrs[compnum][0][0], 
             coef_buffers[compnum][rownum][0],
             block_row_size[compnum]);
    }
  }

  /* Write to the output file */
  jpeg_write_coefficients(&outputinfo, coef_arrays);

  /* Finish compression and release memory */
  jpeg_finish_compress(&outputinfo);
  jpeg_destroy_compress(&outputinfo);
  jpeg_finish_decompress(&inputinfo);
  jpeg_destroy_decompress(&inputinfo);

  /* Close files */
  fclose(input_file);
  fclose(output_file);

  /* All done. */
  printf("New DCT coefficients successfully written to %s\n\n", outputname);
  exit(jerr.num_warnings ? EXIT_WARNING : EXIT_SUCCESS);
  return 0;     /* suppress no-return-value warnings */
}
