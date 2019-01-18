#pragma once
#include <vtkType.h>
#include <string.h> // memcpy_s—p

class vtkDataSet;
class vtkUnstructuredGrid;
class vtkDoubleArray;
class vtkDataArray;

class FDMSolverForDiffusionEq
{
public:
  FDMSolverForDiffusionEq() {};
  ~FDMSolverForDiffusionEq() {};

  void SolveHeatEquationForAllStep(vtkUnstructuredGrid *out_field, const int &in_endTime);
  bool SolveHeatEquationForOneStep(vtkUnstructuredGrid *out_field);
  void SetParameters(vtkUnstructuredGrid *out_field, const double &in_alpha);
  void SetAveValueFromSurroundings(const vtkIdType in_voxelId, vtkDoubleArray * out_results, vtkDataArray * in_properties);
  void SetBoundryCondition(vtkDoubleArray *in_results, vtkDataArray *in_properties);

  // setter
  void SetAlpha(const double in_val) { alpha = in_val; }
  void SetWallXYZBC(const double *in_vals) { memcpy_s(wallXYZBC, sizeof(double) * 3, in_vals, sizeof(double) * 3); }
  void SetWallXBC(const double in_val) { wallXYZBC[0] = in_val; }
  void SetWallYBC(const double in_val) { wallXYZBC[1] = in_val; }
  void SetWallZBC(const double in_val) { wallXYZBC[2] = in_val; }

private:
  double lengths[3];
  int cellDims[3];
  double alpha;
  double deltas[3];
  double dt;
  double wallXYZBC[3];
};

