#include <iostream>

#include <vtkSelectEnclosedPoints.h>

#include <vtkGeometryFilter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellCenters.h>
#include <vtkSphereSource.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkThreshold.h>
#include <vtkStructuredGrid.h>
#include <vtkAppendFilter.h>

#include <vtkDataSetReader.h>
#include <vtkDataSetWriter.h>

#include <vtkSmartPointer.h>

void GenerateBaseMeshFromAABB(vtkDataSet *out_baseMesh, vtkPolyData *in_polyData, const int in_cellDims[3]);

int main()
{
  // �@ �Ȗʃ|���S���f�[�^����� ----
  auto reader = vtkSmartPointer<vtkDataSetReader>::New();
  reader->SetFileName("C:\\test\\ushi.vtk");
  //reader->SetFileName("C:\\test\\fillHole.vtk");
  reader->Update();

  // �\���b�h���T�[�t�F�X�ɕς���B���X�T�[�t�F�X�Ȃ�Ȃ���ς��Ȃ�
  auto geometry = vtkSmartPointer<vtkGeometryFilter>::New();  
  geometry->SetInputData(reader->GetOutput());
  geometry->Update();

  vtkPolyData *closedPoly = geometry->GetOutput(); // polydata�̃|�C���^�Ō`����Ǘ�(���Ȃ��Ă�����)
  // ----

  // �A ��b���b�V������� ----
  auto baseMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();  // ��b���b�V��
  const int cellDims[] = { 50, 50, 50 };
  GenerateBaseMeshFromAABB(baseMesh, closedPoly, cellDims);
  // ----

  // �B �_�Q�f�[�^����� ----
  // �S�{�N�Z�����S���W���{�N�Z�����������
  auto cellCenters = vtkSmartPointer<vtkCellCenters>::New();
  cellCenters->SetInputData(baseMesh);
  cellCenters->Update();  // �S�{�N�Z�����S���W���擾

  vtkPolyData *polyPoints = cellCenters->GetOutput(); // polydata�̃|�C���^�Ō`����Ǘ�(���Ȃ��Ă�����)
  // ----

  // �C vtkSelectEnclosedPoints�œ��O���� ----
  auto selectEnclosedPts = vtkSmartPointer<vtkSelectEnclosedPoints>::New();
  selectEnclosedPts->SetInputData(polyPoints);    // �_�Q�Z�b�g
  selectEnclosedPts->SetSurfaceData(closedPoly);  // �ȖʃZ�b�g
  selectEnclosedPts->SetTolerance(1e-5);
  selectEnclosedPts->Update();
  // ----

  // �D �t�B���^�K�p�A�t�B���^�����`��ɂԂ牺�����Ă���
  // "SelectedPoints"�Ƃ����z����Q�Ƃ���baseMesh��CellData�ɗ^����
  vtkDataArray *isInsideOrOutside = selectEnclosedPts->GetOutput()->GetPointData()->GetArray("SelectedPoints");
  baseMesh->GetCellData()->AddArray(isInsideOrOutside); // PointData�Ŕz����擾������ACellData�ɂԂ炳����̂Œ���

  // �`���CellData�ɂԂ牺�����Ă���"SelectedPoints"�Ƃ����z�񂩂���𒊏o����
  auto threshold = vtkSmartPointer<vtkThreshold>::New();
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "SelectedPoints");
  threshold->SetInputData(baseMesh);
  threshold->ThresholdBetween(1, 1);  // (min, max) = (1, 1)
  threshold->Update();

  // �E VTK���K�V�[�`���ŏo��
  auto writer = vtkSmartPointer<vtkDataSetWriter>::New();
  writer->SetInputData(threshold->GetOutput());
  writer->SetFileName("C:\\test\\voxelMesh.vtk");
  writer->Update();
}

// �Ȗʂ�AABB(Axis-Aligned Bounding Box)�����b���b�V�����쐬
// ��1�����ɋ�̌`��f�[�^��^���邱��
void GenerateBaseMeshFromAABB(vtkDataSet *out_baseMesh, vtkPolyData *in_polyData, const int in_cellDims[3])
{
  // ����
  auto bounds = in_polyData->GetBounds();
  const double meshPitches[3] =
  {
    (bounds[1] - bounds[0]) / in_cellDims[0],
    (bounds[3] - bounds[2]) / in_cellDims[1],
    (bounds[5] - bounds[4]) / in_cellDims[2]
  };
  const double mins[3] = { bounds[0], bounds[2], bounds[4] };

  // ���Ԋu�i�q�_����
  auto points = vtkSmartPointer<vtkPoints>::New();
  for (int zId = 0; zId < in_cellDims[2] + 1; zId++)
  {
    for (int yId = 0; yId < in_cellDims[1] + 1; yId++)
    {
      for (int xId = 0; xId < in_cellDims[0] + 1; xId++)
      {
        const double x = xId * meshPitches[0] + mins[0];
        const double y = yId * meshPitches[1] + mins[1];
        const double z = zId * meshPitches[2] + mins[2];
        points->InsertNextPoint(x, y, z);
      }
    }
  }

  // �Ȗʂ�����AABB���ɍ\���i�q�𐶐�
  auto baseMesh = vtkSmartPointer<vtkStructuredGrid>::New();
  baseMesh->SetExtent(0, in_cellDims[0], 0, in_cellDims[1], 0, in_cellDims[2]);
  baseMesh->SetPoints(points);

  // �����Ǝg���ɂ������Ƃ�����̂ŁAvtkStructuredGrid��vtkUnstructuredGrid�ɕϊ�
  auto append = vtkSmartPointer<vtkAppendFilter>::New();
  append->AddInputData(baseMesh);
  append->Update();

  // ��1�����ɃR�s�[
  out_baseMesh->DeepCopy(append->GetOutput());
}