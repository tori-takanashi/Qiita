#include "VoxelMeshGeneratorByVtk.h"
#include "SettingAnalyticProperties.h"
#include "FDMSolverForHeatEq.h"

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
  // �Ȗʓǂݎ��
  auto reader = vtkSmartPointer<vtkDataSetReader>::New();
  reader->SetFileName("C:\\test\\bunny.vtk");
  reader->Update();

  // PolyData�ɕϊ�
  auto geometry = vtkSmartPointer<vtkGeometryFilter>::New();
  geometry->SetInputData(reader->GetOutput());
  geometry->Update();

  cout << "�{�N�Z�����b�V���쐬" << endl;
  shared_ptr<VoxelMeshGeneratorByVtk> voxelMesh = make_shared<VoxelMeshGeneratorByVtk>();
  voxelMesh->SetInputData(geometry->GetOutput());

  const int cellDims[] = { 60, 60, 60 };
  const double *bounds = geometry->GetOutput()->GetBounds();
  const double offsetX = (bounds[1] - bounds[0]) * 0.2;
  const double offsetY = (bounds[3] - bounds[2]) * 0.2;
  const double offsetZ = (bounds[5] - bounds[4]) * 0.2;
  const double offsets[6] = { offsetX, offsetX, offsetY, offsetY, offsetZ, offsetZ };

  voxelMesh->SetCellDimension(cellDims);
  voxelMesh->SetOffsets(offsets);
  voxelMesh->Update();

  cout << "�{�N�Z�����b�V���ɉ�͑�����t�^" << endl;
  shared_ptr<SettingAnalyticProperties> voxelMeshWithProperties = make_shared<SettingAnalyticProperties>();
  voxelMeshWithProperties->SetCellDimension(cellDims);
  voxelMeshWithProperties->SetInputData(voxelMesh->GetOutput());
  voxelMeshWithProperties->Update();

  auto field = vtkSmartPointer<vtkUnstructuredGrid>::New();
  field->DeepCopy(voxelMeshWithProperties->GetOutput());

  cout << "��͊J�n" << endl;
  shared_ptr<FDMSolverForHeatEq> solver = make_shared<FDMSolverForHeatEq>();

  const double alpha = 0.1;
  const double wallXYZBC[3] = { 10.0, 100.0, 40.0 };
  const int endStep = 200;

  solver->SetAlpha(alpha);
  solver->SetWallXYZBC(wallXYZBC);
  solver->SolveHeatEquationForAllStep(field, endStep);

  {
    auto writer = vtkSmartPointer<vtkXMLDataSetWriter>::New();
    writer->SetFileName("C:\\test\\Results.vtu");
    writer->SetInputData(field);
    writer->Update();
  }

  return 0;
}