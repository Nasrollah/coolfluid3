// file automatically generated - do not edit!
// inplace solve A^T \\ B
// matrix layouts: A...col_major, B...col_major
__kernel void trans_lower_solve(
          __global const float * A,
          unsigned int A_rows,
          unsigned int A_cols,
          unsigned int A_internal_rows,
          unsigned int A_internal_cols,
          __global float * B,  
          unsigned int B_rows,
          unsigned int B_cols,
          unsigned int B_internal_rows,
          unsigned int B_internal_cols)
{ 
  float temp; 
  for (int row = 0; row < A_rows; ++row) 
  { 
    barrier(CLK_GLOBAL_MEM_FENCE); 
    if (get_local_id(0) == 0) 
      B[row + get_group_id(0) * B_internal_rows] /= A[row + row*A_internal_cols]; 
    barrier(CLK_GLOBAL_MEM_FENCE); 
      temp = B[row + get_group_id(0) * B_internal_rows]; 
    //eliminate column of op(A) with index 'row' in parallel: 
    for  (int elim = row + get_local_id(0) + 1; elim < A_rows; elim += get_local_size(0)) 
      B[elim + get_group_id(0) * B_internal_rows] -= temp * A[elim * A_internal_rows + row];
   }
}
