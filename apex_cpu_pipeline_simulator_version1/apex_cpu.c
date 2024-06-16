/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
//final dimple 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }
    
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_JALR:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }
        
       
        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_LOADP:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_STORE:
        case OPCODE_STOREP:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        case OPCODE_BN:
        case OPCODE_BNN:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }
        

        case OPCODE_HALT:
        {
            printf("%s", stage->opcode_str);
            break;
        }
        case OPCODE_NOP:
        {
            printf("%s", stage->opcode_str);
            break;
        }
        case OPCODE_CML:
        case OPCODE_JUMP:
        {
            printf("%s,R%d,#%d", stage->opcode_str,stage->rs1,stage->imm);
            break;
        }
        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d", stage->opcode_str,stage->rs1,stage->rs2);
            break;
        }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{

    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }
    printf("\n \n \n");
    printf("----------\n%s\n----------\n", "FLAGS (+,-,0):");
    printf("\n Zero flag = %d", cpu->cc.z);
    printf("\n Positive flag = %d", cpu->cc.p);
    printf("\n Negative flag = %d", cpu->cc.n);
    printf("\n");
    printf("\n \n \n");
    printf("----------\n%s\n----------\n", "MEMORY:");
    for (int i =0; i<cpu->data_counter; ++i){
        printf("\nMemory[%d]= %d\n", cpu->mem_address[i],cpu->data_memory[cpu->mem_address[i]]);
    }
    printf("\n\n");
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (cpu->fetch.has_insn)
    {

        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

          /* Store current PC in fetch latch */
          cpu->fetch.pc = cpu->pc;

          /* Index into code memory using this pc and copy all instruction fields
           * into fetch latch  */
          current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
          strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
          cpu->fetch.opcode = current_ins->opcode;
          cpu->fetch.rd = current_ins->rd;
          cpu->fetch.rs1 = current_ins->rs1;
          cpu->fetch.rs2 = current_ins->rs2;
          cpu->fetch.imm = current_ins->imm;
          if(cpu->fetch.stalled == 0){
            /* Update PC for next instruction */
            cpu->pc += 4;

            /* Copy data from fetch latch to decode latch*/

              cpu->decode = cpu->fetch;
        }

        if(cpu->fetch.stalled == 0){

            /* Stop fetching new instructions if HALT is fetched */
            if (cpu->fetch.opcode == OPCODE_HALT)
            {
                cpu->fetch.has_insn = FALSE;
            }
        }
        if (ENABLE_DEBUG_MESSAGES)
    {
        print_stage_content("Fetch", &cpu->fetch);
    }
    }
    
    
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{
    if (cpu->decode.has_insn)
    {

        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
            case OPCODE_ADD:
            {
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->regs_writing[cpu->decode.rs2] == 0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                  cpu->regs_writing[cpu->decode.rd] = 1;
                  cpu->decode.stalled = 0;
                  break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }
            }
            case OPCODE_ADDL:
            case OPCODE_JALR:
                if (cpu->regs_writing[cpu->decode.rs1] == 0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->regs_writing[cpu->decode.rd] = 1;
                  cpu->decode.stalled = 0;
                  break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }

            case OPCODE_JUMP:
                if (cpu->regs_writing[cpu->decode.rs1] == 0){
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    cpu->decode.stalled = 0;
                    break;
                }
                else{
                    cpu->decode.stalled = 1;
                    break;
                }

            case OPCODE_SUB:
            {
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->regs_writing[cpu->decode.rs2] == 0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                  cpu->regs_writing[cpu->decode.rd] = 1;
                  cpu->decode.stalled = 0;
                  break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }

            }
            case OPCODE_SUBL:
                {
                if (cpu->regs_writing[cpu->decode.rs1] == 0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->regs_writing[cpu->decode.rd] = 1;
                  cpu->decode.stalled = 0;
                  break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }

            }
            case OPCODE_MUL:
            {
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->regs_writing[cpu->decode.rs2] == 0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                  cpu->regs_writing[cpu->decode.rd] = 1;
                  cpu->decode.stalled = 0;
                  break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }

            }
            case OPCODE_AND:
            {
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->regs_writing[cpu->decode.rs2] == 0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                  cpu->regs_writing[cpu->decode.rd] = 1;
                  cpu->decode.stalled = 0;
                  break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }

            }
            case OPCODE_OR:
            {
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->regs_writing[cpu->decode.rs2] == 0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                  cpu->regs_writing[cpu->decode.rd] = 1;
                  cpu->decode.stalled = 0;
                  break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }

            }
            case OPCODE_XOR:
            {
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->regs_writing[cpu->decode.rs2] == 0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                  cpu->regs_writing[cpu->decode.rd] = 1;
                  cpu->decode.stalled = 0;
                  break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }

            }

            case OPCODE_LOAD:
            {
                if (cpu->regs_writing[cpu->decode.rs1]==0){
                    cpu->decode.stalled=0;
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    break;
                }
                
            }
            case OPCODE_LOADP:
            {   
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->regs_writing[cpu->decode.rs2] == 0){
                     cpu->decode.stalled = 0;
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    break;
                }
                 else
                {
                    cpu->decode.stalled=1;
                    break;
                }
            }
            case OPCODE_STORE:
              if (cpu->regs_writing[cpu->decode.rs1] == 0){
                    cpu->decode.stalled = 0;
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                    break;
                }
                else
                {
                    cpu->decode.stalled=1;
                    break;
                }
            case OPCODE_STOREP:
            {
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->regs_writing[cpu->decode.rs2] == 0){
                    cpu->decode.stalled = 0;
                    cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                    cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                    cpu->regs_writing[cpu->decode.rs1] = 1;
                    cpu->regs_writing[cpu->decode.rs2] = 1;

                    break;
                }
                else
                {
                    cpu->decode.stalled=1;
                    break;
                }
            }

            case OPCODE_MOVC:
            {
                /* MOVC doesn't have register operands */
                cpu->regs_writing[cpu->decode.rd] = 1;
                break;
            }
            case OPCODE_CML:
            {
                if (cpu->regs_writing[cpu->decode.rs1] == 0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.stalled = 0;
                  break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }
            }
            case OPCODE_CMP:
            {
                if (cpu->regs_writing[cpu->decode.rs1] == 0 && cpu->regs_writing[cpu->decode.rs2] == 0){
                  cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                  cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                  cpu->decode.stalled = 0;
                  break;
                }
                else{
                  cpu->decode.stalled = 1;
                  break;
                }
            }
        }

        /* Copy data from decode latch to execute latch*/

        if (cpu->decode.stalled == 0){
          cpu->execute = cpu->decode;
          cpu->fetch.stalled = 0;
        }
        else{
          cpu->fetch.stalled = 1;
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/RF", &cpu->decode);
        }
    }
    
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
          case OPCODE_ADD:
          {
                if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
              cpu->execute.result_buffer
                  = cpu->execute.rs1_value + cpu->execute.rs2_value;

              /* Set the zero flag based on the result buffer */
              if(cpu->execute.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->execute.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
               
              break;
          }
          case OPCODE_ADDL:
          {
            if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
              cpu->execute.result_buffer
                  = cpu->execute.rs1_value + cpu->execute.imm;
              /* Set the zero flag based on the result buffer */
              if(cpu->execute.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->execute.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
                
              break;
          }
          case OPCODE_SUB:
          {
            if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
              cpu->execute.result_buffer
                  = cpu->execute.rs1_value - cpu->execute.rs2_value;
              
              /* Set the zero flag based on the result buffer */
             if(cpu->execute.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->execute.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
                
              break;
          }
          case OPCODE_SUBL:
          {
            if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
              cpu->execute.result_buffer
                  = cpu->execute.rs1_value - cpu->execute.imm;
              
              /* Set the zero flag based on the result buffer */
              if(cpu->execute.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->execute.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
                
              break;
          }
          case OPCODE_MUL:
          {
            if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
              cpu->execute.result_buffer
                  = cpu->execute.rs1_value * cpu->execute.rs2_value;

              /* Set the zero flag based on the result buffer */
              if(cpu->execute.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->execute.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
              break;
          }
          case OPCODE_AND:
          {
              if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
              cpu->execute.result_buffer
                  = cpu->execute.rs1_value&cpu->execute.rs2_value;
                
              /* Set the zero flag based on the result buffer */
              if(cpu->execute.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->execute.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
                
              break;
          }
          case OPCODE_OR:
          {
            if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
              cpu->execute.result_buffer
                  = cpu->execute.rs1_value | cpu->execute.rs2_value;

              /* Set the zero flag based on the result buffer */
              if(cpu->execute.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->execute.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
                
              break;
          }
          case OPCODE_XOR:
          {
            if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
              cpu->execute.result_buffer
                  = cpu->execute.rs1_value ^ cpu->execute.rs2_value;

              /* Set the zero flag based on the result buffer */
              if(cpu->execute.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                if(cpu->execute.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
                
              break;
          }


            case OPCODE_LOAD:
            {
                if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
                cpu->execute.memory_address
                    = cpu->execute.rs1_value + cpu->execute.imm;
                break;
            }
            case OPCODE_LOADP:
            {
                if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
              if(cpu->regs_writing[cpu->execute.rs1] == 0){
                cpu->regs_writing[cpu->execute.rs1] = 1;
              }
                cpu->execute.memory_address
                    = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->execute.rs1_value=cpu->execute.rs1_value+4;
                break;
            }
            case OPCODE_STORE:
            {
                cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
                cpu->mem_address[cpu->data_counter++] = cpu->execute.memory_address;
                break;
            }
            case OPCODE_STOREP:
            {
                if(cpu->regs_writing[cpu->execute.rs2] == 0){
                cpu->regs_writing[cpu->execute.rs2] = 1;
              }
                cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
                cpu->execute.rs2_value =cpu->execute.rs2_value +4;
                cpu->mem_address[cpu->data_counter++] = cpu->execute.memory_address;

                break;
            }
            case OPCODE_BZ:
            {
                if (cpu->zero_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;

                    /* Since we are using reverse callbacks for pipeline stages,
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BNZ:
            {
                if (cpu->zero_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;

                    /* Since we are using reverse callbacks for pipeline stages,
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }
            case OPCODE_BN:
            {
                if (cpu->cc.n == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;

                    /* Since we are using reverse callbacks for pipeline stages,
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }
            case OPCODE_BNN:
            {
                if (cpu->cc.n == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;

                    /* Since we are using reverse callbacks for pipeline stages,
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }
            case OPCODE_BP:
            {
                if (cpu->cc.p == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;

                    /* Since we are using reverse callbacks for pipeline stages,
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }
            case OPCODE_BNP:
            {
                if (cpu->cc.p == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;

                    /* Since we are using reverse callbacks for pipeline stages,
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }
            case OPCODE_JUMP:
            {
                /* Calculate new PC, and send it to fetch unit */
                cpu->pc = cpu->execute.rs1_value + cpu->execute.imm;

                /* Since we are using reverse callbacks for pipeline stages,
                    * this will prevent the new instruction from being fetched in the current cycle*/
                cpu->fetch_from_next_cycle = TRUE;

                /* Flush previous stages */
                cpu->decode.has_insn = FALSE;

                /* Make sure fetch stage is enabled to start fetching from new PC */
                cpu->fetch.has_insn = TRUE;
            
                break;
            }
            case OPCODE_JALR:
            {
                if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
                /* Calculate new PC, and send it to fetch unit */
                cpu->execute.result_buffer = cpu->execute.pc + 4;
                cpu->pc = cpu->execute.rs1_value + cpu->execute.imm;

                /* Since we are using reverse callbacks for pipeline stages,
                    * this will prevent the new instruction from being fetched in the current cycle*/
                cpu->fetch_from_next_cycle = TRUE;

                /* Flush previous stages */
                cpu->decode.has_insn = FALSE;

                /* Make sure fetch stage is enabled to start fetching from new PC */
                cpu->fetch.has_insn = TRUE;
            
                break;
            }

            case OPCODE_MOVC:
            {
                if(cpu->regs_writing[cpu->execute.rd] == 0){
                cpu->regs_writing[cpu->execute.rd] = 1;
              }
                cpu->execute.result_buffer = cpu->execute.imm;
               
                break;
            }
            case OPCODE_CML:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value-cpu->execute.imm;
                
                if(cpu->execute.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                else if(cpu->execute.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                else if (cpu->execute.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }
                break;
            }
            case OPCODE_CMP:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value-cpu->execute.rs2_value;
                
                if(cpu->execute.result_buffer<0){
                    cpu->cc.p = FALSE;
                    cpu->cc.n = TRUE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }
                else if(cpu->execute.result_buffer>0){
                    cpu->cc.p = TRUE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = FALSE;
                    cpu->zero_flag = FALSE;
                }

                /* Set the zero flag based on the result buffer */
                else if (cpu->execute.result_buffer == 0)
                {
                    cpu->cc.p = FALSE;
                    cpu->cc.n = FALSE;
                    cpu->cc.z = TRUE;
                    cpu->zero_flag = TRUE;
                }

                break;
            }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

         if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }
   
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            {
                /* No work for ADD */
                if(cpu->regs_writing[cpu->memory.rd] == 0){
                cpu->regs_writing[cpu->memory.rd] = 1;
              }
                break;
            }

            case OPCODE_LOAD:
            {
                if(cpu->regs_writing[cpu->memory.rd] == 0){
                cpu->regs_writing[cpu->memory.rd] = 1;
              }
              cpu->memory.result_buffer
                    = cpu->data_memory[cpu->memory.memory_address];
                break;
            }
            case OPCODE_LOADP:
            {
                if(cpu->regs_writing[cpu->memory.rd] == 0){
                cpu->regs_writing[cpu->memory.rd] = 1;
              }
              if(cpu->regs_writing[cpu->memory.rs1] == 0){
                cpu->regs_writing[cpu->memory.rs1] = 1;
              }
                /* Read from data memory */
                cpu->memory.result_buffer
                    = cpu->data_memory[cpu->memory.memory_address];
                break;
            }
            case OPCODE_STORE:
            {
                {
                /* Read from data memory */
                cpu->data_memory[cpu->memory.memory_address]= cpu->memory.rs1_value;
                //cpu->mem_val= cpu->data_memory[cpu->memory.memory_address];
                break;
            }
            }
            case OPCODE_STOREP:
            {
                if(cpu->regs_writing[cpu->memory.rs2] == 0){
                cpu->regs_writing[cpu->memory.rs2] = 1;
              }

                /* Read from data memory */
                cpu->data_memory[cpu->memory.memory_address]= cpu->memory.rs1_value;
                //cpu->mem_val= cpu->data_memory[cpu->memory.memory_address];
                break;
            }
            case OPCODE_JALR:
            {
                if(cpu->regs_writing[cpu->memory.rd] == 0){
                  cpu->regs_writing[cpu->memory.rd] = 1;
                }
            }
            case OPCODE_JUMP:
            {
                break;
            }
        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

         if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
   
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
          case OPCODE_ADD:
          case OPCODE_ADDL:
          case OPCODE_SUB:
          case OPCODE_SUBL:
          case OPCODE_MUL:
          case OPCODE_AND:
          case OPCODE_OR:
          case OPCODE_XOR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regs_writing[cpu->writeback.rd] = 0;
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regs_writing[cpu->writeback.rd] = 0;

                break;
            }
            case OPCODE_LOADP:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regs[cpu->writeback.rs1]=cpu->writeback.rs1_value;                
                cpu->regs_writing[cpu->writeback.rd] = 0;
                cpu->regs_writing[cpu->writeback.rs1] = 0;
                break;
            }
            case OPCODE_STOREP:
            {
                cpu->regs[cpu->writeback.rs2]=cpu->writeback.rs2_value;
                cpu->regs_writing[cpu->writeback.rd] = 0;
                cpu->regs_writing[cpu->writeback.rs2] = 0;

                break;
            }

            case OPCODE_MOVC:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regs_writing[cpu->writeback.rd] = 0;
                break;
            }
            case OPCODE_JALR:
            {
                cpu->regs[cpu->writeback.rd]= cpu->writeback.result_buffer;
                cpu->regs_writing[cpu->writeback.rd] = 0;
                break;
            }
            case OPCODE_HALT:
            {
                break;
            }
        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

       
        if (cpu->writeback.opcode == OPCODE_HALT)
            {
                /* Stop the APEX simulator */
                return TRUE;
            }
         if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }
    }
   
    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;
    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;
    cpu->simulate= SIMULATE_STEP;
    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }
    cpu->data_counter = 0;

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */

void simulate_cpu_for_cycles(APEX_CPU *cpu, int num_cycles) {
    for (int cycle = 1; cycle <= num_cycles; cycle++) {
        if (ENABLE_DEBUG_MESSAGES) {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cycle);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu)) {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cycle, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);

    }
}

void
APEX_cpu_run(APEX_CPU *cpu)
{

    char user_prompt_val;
    cpu->clock =1;
    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}
