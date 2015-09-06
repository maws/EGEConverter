#include <fbxsdk.h>
#include <iostream>
#include <fstream> 
#include <stdio.h>
#include <string>

int main(int argc, char *argv[])
{
	// The master vertices list used for exporting
	float* vertices = nullptr; // Allocate when we know length
	unsigned numVerts = 0;

	std::string fileName = "cube.fbx";
	printf("Importing %s \n", fileName.c_str());
	
	// Strip extension
	int extIndex = fileName.find_last_of(".");
	std::string fileNameRaw = fileName.substr(0, extIndex);

	FbxManager* manager = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
	FbxScene* scene = FbxScene::Create(manager, "Scene");

	// Get sdk version info
	int sdkMajor, sdkMinor, sdkRev;
	FbxManager::GetFileFormatVersion(sdkMajor, sdkMinor, sdkRev);

	FbxImporter* importer = FbxImporter::Create(manager, "");
	bool status = importer->Initialize(fileName.c_str(), -1, manager->GetIOSettings());

	// Get file version info
	int fileMajor, fileMinor, fileRev;
	importer->GetFileVersion(fileMajor, fileMinor, fileRev);

	if (!status)
	{
		// Print error
		FbxString error = importer->GetStatus().GetErrorString();
		printf("Failed to initialize fbx importer. \n");
		printf("Error: %s \n", error.Buffer());
	}

	printf("FBX SDK version: %d.%d.%d \n", sdkMajor, sdkMinor, sdkRev);
	if (importer->IsFBX())
	{
		printf("FBX file format version: %d.%d.%d \n", fileMajor, fileMinor, fileRev);

	}

	// Import scene
	status = importer->Import(scene);

	auto rootNode = scene->GetRootNode();
	
	if (rootNode)
	{
		for (int i = 0; i < rootNode->GetChildCount(); ++i)
		{
			auto node = rootNode->GetChild(i);

			FbxNodeAttribute::EType type;
			if (node->GetNodeAttribute() == nullptr)
			{
				printf("Node %d has no attribute \n", i);
			}
			else
			{
				type = node->GetNodeAttribute()->GetAttributeType();

				switch (type)
				{
				default:
					break;
				case FbxNodeAttribute::eMesh:
					{
						FbxMesh* mesh = (FbxMesh*)node->GetNodeAttribute();
						printf("Mesh name: %s \n", (char*)node->GetName());
						numVerts = mesh->GetPolygonVertexCount() * 3;
						vertices = new float[numVerts];
						auto polyCount = mesh->GetPolygonCount();
						FbxVector4* controlPoints = mesh->GetControlPoints();

						// Loop polys
						for (int poly = 0; poly < polyCount; ++poly)
						{
							auto polySize = mesh->GetPolygonSize(poly);
							for (int polyVertex = 0; polyVertex < polySize; ++polyVertex)
							{
								int controlPointIndex = mesh->GetPolygonVertex(poly, polyVertex);
								FbxVector4 coordinate = controlPoints[controlPointIndex];
								printf("Polygon %d vertex %d has coordinate %f %f %f \n", poly, polyVertex, coordinate[0], coordinate[1], coordinate[2]);

								// Trying to keep it platforms friendly here
								// e.g. The Vertex struct can be padded differently
								// So copy each element one at a time instead of whole Vertex struct
								int dstIndex = (poly * polySize * 3) + (polyVertex * 3);
								float currentPolyVerts[3] = { coordinate[0], coordinate[1], coordinate[2] };
								memcpy(&vertices[dstIndex], currentPolyVerts, sizeof(currentPolyVerts));
							}
						}
					}
					break;
				}
			}
		}
	}

	printf("Total vertices: %d \n", numVerts);
	printf("Exporting to e3m... \n");
	fileNameRaw.append(".e3m");

	float b[72];
	for (int i = 0; i < 72; ++i)
		b[i] = vertices[i];

	{ // Write
		FILE* file;
		file = fopen(fileNameRaw.c_str(), "wb");
		// Write the number of vertices
		fwrite(&numVerts, sizeof(unsigned), 1, file);
		// Write vertices
		fwrite(vertices, sizeof(float), numVerts, file);
		fclose(file);
	}
	{ // Read
		float* buffer = new float[numVerts];
		FILE* file;
		file = fopen(fileNameRaw.c_str(), "rb");
		fread(&numVerts, sizeof(unsigned), 1, file);
		fread(buffer, sizeof(float), numVerts, file);
		fclose(file);

		float b[72];
		for (int i = 0; i < 72; ++i)
			b[i] = buffer[i];
		delete buffer;
	}

	printf("...Finished: Cleaning up. \n");
	delete vertices;
	manager->Destroy();
	return 0;
}
