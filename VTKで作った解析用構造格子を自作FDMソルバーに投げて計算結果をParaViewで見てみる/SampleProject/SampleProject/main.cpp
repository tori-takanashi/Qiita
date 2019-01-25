#include "vtkVoxelMeshGenerator.h"
#include "vtkSettingAnalyticProperties.h"
#include "FDMSolverForDiffusionEq.h"

#include <iostream>
#include <memory>

#include <vtkGeometryFilter.h>
#include <vtkUnstructuredGrid.h>

// Reader & Writer
#include <vtkDataSetReader.h>
#include <vtkSTLReader.h>
#include <vtkDataSetWriter.h>
#include <vtkXMLDataSetWriter.h>

using namespace std;

int main()
{
  // 閉曲面読み取り
  auto reader = vtkSmartPointer<vtkDataSetReader>::New();
  reader->SetFileName("C:\\sandbox\\bunny.vtk");
  reader->Update();

  // PolyDataに変換
  auto geometry = vtkSmartPointer<vtkGeometryFilter>::New();
  geometry->SetInputData(reader->GetOutput());
  geometry->Update();

  cout << "ボクセルメッシュ作成" << endl;
  const int cellDims[] = { 60, 60, 60 };
  const double *bounds = geometry->GetOutput()->GetBounds();
  const double offsetX = (bounds[1] - bounds[0]) * 0.2;
  const double offsetY = (bounds[3] - bounds[2]) * 0.2;
  const double offsetZ = (bounds[5] - bounds[4]) * 0.2;
  const double offsets[6] = { offsetX, offsetX, offsetY, offsetY, offsetZ, offsetZ };

  // スマートポインタでインスタンスを管理
  shared_ptr<vtkVoxelMeshGenerator> voxelMeshGenerator = make_shared<vtkVoxelMeshGenerator>();
  voxelMeshGenerator->SetInputData(geometry->GetOutput());

  voxelMeshGenerator->SetCellDimension(cellDims);
  voxelMeshGenerator->SetOffsets(offsets);
  voxelMeshGenerator->Update();

  cout << "ボクセルメッシュに解析属性を付与" << endl;
  shared_ptr<vtkSettingAnalyticProperties> analyticProperties;
  analyticProperties = make_shared<vtkSettingAnalyticProperties>();

  analyticProperties->SetCellDimension(cellDims);
  analyticProperties->SetInputData(voxelMeshGenerator->GetOutput());
  analyticProperties->Update();

  auto field = vtkSmartPointer<vtkUnstructuredGrid>::New();
  field->DeepCopy(analyticProperties->GetOutput());

  cout << "解析開始" << endl;
  const double alpha = 0.1;
  const double wallXYZBC[3] = { 10.0, 100.0, 40.0 };
  const int endStep = 2000;

  shared_ptr<FDMSolverForDiffusionEq> solver;
  solver = make_shared<FDMSolverForDiffusionEq>();

  solver->SetAlpha(alpha);
  solver->SetWallXYZBC(wallXYZBC);
  solver->SolveHeatEquationForAllStep(field, endStep);

  //auto writer = vtkSmartPointer<vtkXMLDataSetWriter>::New();
  //writer->SetFileName("C:\\sandbox\\Results.vtu");
  //writer->SetInputData(field);
  //writer->Update();

  return 0;
}