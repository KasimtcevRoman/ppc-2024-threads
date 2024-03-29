// Copyright 2024 Sadikov Damir
#include "seq/sadikov_d_crs_mult/include/sadikov_d_seq.hpp"

#include <algorithm>
#include <complex>
#include <utility>
#include <vector>

// TODO delete iosteam
#include <iostream>

bool check_CRS_prproperties(const matrix_CRS& A) {
  if (A.row_id.size() != A.n + 1) return false;
  int nz = A.value.size();
  if (A.value.size() != nz || A.col.size() != nz || A.row_id[A.n] != nz) return false;
  if (A.row_id[0] != 0) return false;
  for (int i = 1; i <= A.n; i++) {
    if (A.row_id[i] < A.row_id[i - 1]) return false;
  }
  for (int i = 0; i < nz; i++) {
    if (A.col[i] < 0 || A.col[i] >= A.m) return false;
  }
  return true;
}

matrix_CRS transpose_CRS(const matrix_CRS& B) {
  matrix_CRS B_T;
  B_T.n = B.m;
  B_T.m = B.n;
  B_T.row_id.assign(B_T.n + 1, 0);
  std::vector<std::pair<int, std::pair<int, std::complex<double>>>> temp;
  for (int i = 0; i < B.n; i++) {
    for (int k = B.row_id[i]; k < B.row_id[i + 1]; k++) {
      int j = B.col[k];
      temp.push_back({j, {i, B.value[k]}});
    }
  }
  std::sort(temp.begin(), temp.end(),
            [&](const std::pair<int, std::pair<int, std::complex<double>>>& a,
                const std::pair<int, std::pair<int, std::complex<double>>>& b) {
              if (a.first == b.first) return a.second.first < b.second.first;
              return a.first < b.first;
            });
  for (auto& i : temp) {
    B_T.row_id[i.first + 1]++;
    B_T.col.push_back(i.second.first);
    B_T.value.push_back(i.second.second);
  }
  for (int i = 0; i < B_T.n; i++) {
    B_T.row_id[i + 1] += B_T.row_id[i];
  }
  return B_T;
}

bool CRSComplexMult_Sequential::validation() {
  internal_order_test();
  // check taskData vectors sizes
  if (taskData->inputs.size() != 2 || taskData->outputs.size() != 1 || !taskData->inputs_count.empty() ||
      !taskData->outputs_count.empty())
    return false;
  // check for nullptr
  if (taskData->inputs[0] == nullptr || taskData->inputs[1] == nullptr || taskData->outputs[0] == nullptr) return false;
  A = reinterpret_cast<matrix_CRS*>(taskData->inputs[0]);
  B = reinterpret_cast<matrix_CRS*>(taskData->inputs[1]);
  C = reinterpret_cast<matrix_CRS*>(taskData->outputs[0]);
  if (A == nullptr || B == nullptr || C == nullptr) return false;
  // check for CRS properties
  if (check_CRS_prproperties(*A) == false) return false;
  if (check_CRS_prproperties(*B) == false) return false;
  // check for matrices size
  return A->m == B->n;
}

bool CRSComplexMult_Sequential::pre_processing() {
  internal_order_test();
  // convert pointers
  A = reinterpret_cast<matrix_CRS*>(taskData->inputs[0]);
  B = reinterpret_cast<matrix_CRS*>(taskData->inputs[1]);
  C = reinterpret_cast<matrix_CRS*>(taskData->outputs[0]);
  // transpose B
  *B = transpose_CRS(*B);
  // TODO delete iosteam
  std::cout << B->n << " " << B->m << "\n";
  for (auto& i : B->row_id) std::cout << i << " ";
  std::cout << "\n";
  for (auto& i : B->col) std::cout << i << " ";
  std::cout << "\n";
  for (auto& i : B->value) std::cout << i << " ";
  std::cout << "\n";
  return true;
}

bool CRSComplexMult_Sequential::run() {
  internal_order_test();
  // multiply matrcies ans store result in C
  C->n = A->n;
  C->m = B->n;  // not m because B is transposed
  C->row_id.assign(C->n + 1, 0);
  std::vector<std::vector<std::pair<int, std::complex<double>>>> temp(C->n);
  for (int i = 0; i < A->n; i++) {
    for (int j = 0; j < B->n; j++) {
      // C[i][j] = dot_product(A[i], B[j]);
      std::complex<double> T;
      bool T_changed = false;
      for (int k_A = A->row_id[i]; k_A < A->row_id[i + 1]; k_A++) {
        for (int k_B = B->row_id[j]; k_B < B->row_id[j + 1]; k_B++) {
          if (A->col[k_A] == B->col[k_B]) {
            T += A->value[k_A] * B->value[k_B];
            T_changed = true;
          }
        }
      }
      if (T_changed) {
        temp[i].emplace_back(j, T);
      }
    }
  }
  for (int i = 0; i < C->n; i++) {
    C->row_id[i + 1] = C->row_id[i];
    for (auto& j : temp[i]) {
      C->col.push_back(j.first);
      C->value.push_back(j.second);
      C->row_id[i + 1]++;
    }
  }
  // TODO delete iosteam
  std::cout << C->n << " " << C->m << "\n";
  for (auto& i : C->row_id) std::cout << i << " ";
  std::cout << "\n";
  for (auto& i : C->col) std::cout << i << " ";
  std::cout << "\n";
  for (auto& i : C->value) std::cout << i << " ";
  std::cout << "\n";
  return true;
}

bool CRSComplexMult_Sequential::post_processing() {
  internal_order_test();
  return true;
}
