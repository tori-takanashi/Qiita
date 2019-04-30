#include <iostream>

#include <vtkFillHolesFilter.h>

#include <vtkGeometryFilter.h>
#include <vtkPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataNormals.h>

#include <vtkDataSetReader.h>
#include <vtkDataSetWriter.h>

#include <vtkSmartPointer.h>

int main()
{
  // �@ ��������|���S���f�[�^��ǂݍ��� ----
  auto reader = vtkSmartPointer<vtkDataSetReader>::New();
  reader->SetFileName("C:\\test\\ushiThatCutLeg.vtk");
  reader->Update();

  // vtkUnstructuredGrid -> vtkPolyData
  auto geometry = vtkSmartPointer<vtkGeometryFilter>::New();
  geometry->SetInputData(reader->GetOutput());
  geometry->Update();

  vtkPolyData *closedPoly = geometry->GetOutput();
  // ----

  // �A ���𖄂߂� ----
  auto fillHole = vtkSmartPointer<vtkFillHolesFilter>::New();
  fillHole->SetInputData(closedPoly);
  fillHole->SetHoleSize(100.0); // �K���ȃT�C�Y����͂��Ă�������
  fillHole->Update();
  // ----

  // �B �d���ߓ_������ ----
  // �`��̑傫������g�������X�����߂�
  double tol = 0.0;
  double *bds = fillHole->GetOutput()->GetBounds();
  for (int id = 0; id < 3; id++)
    tol += abs(bds[id * 2 + 1] - bds[id * 2]);
  tol = tol / 3.0 * 0.001; // �`��̑傫���̕��ς�0.1%

  auto clean = vtkSmartPointer<vtkCleanPolyData>::New();
  clean->SetInputData(fillHole->GetOutput());
  clean->SetTolerance(tol);
  clean->Update();
  // ----

  // �C VTK���K�V�[�`���ŏo��
  auto writer = vtkSmartPointer<vtkDataSetWriter>::New();
  writer->SetInputData(clean->GetOutput());
  writer->SetFileName("C:\\test\\fillHole.vtk");
  writer->Update();
}