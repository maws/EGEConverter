#include <fbxsdk.h>
#include <iostream>
#include <fstream> 
#include <stdio.h>
#include <string>


using namespace fbxsdk_2015_1;

struct Mesh
{
	Mesh() 
	{ 
		vertices = nullptr; 
		indices = nullptr; 
	}

	~Mesh()
	{
		if (vertices) delete vertices;
		if (indices) delete indices;
	}

	int numVertices;
	float* vertices;
	int numIndices;
	int* indices;
};

int main(int argc, char *argv[])
{
	// The export mesh we are going to fill with data and export
	Mesh* exportMesh;

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

						// Create our export mesh and fill it
						exportMesh = new Mesh();

						// Fill vertices
						exportMesh->numVertices = mesh->GetControlPointsCount();
						exportMesh->vertices = new float[exportMesh->numVertices * 3];
						for (int j = 0; j < exportMesh->numVertices; ++j)
						{
							FbxVector4 point = mesh->GetControlPointAt(j);
							float verts[3] = { point.mData[0], point.mData[1], point.mData[2] };
							printf("Vertex %d:  %.1f %.1f %.1f \n", j, verts[0], verts[1], verts[2]);

							int dstIndex = j * 3;
							exportMesh->vertices[dstIndex + 0] = verts[0];
							exportMesh->vertices[dstIndex + 1] = verts[1];
							exportMesh->vertices[dstIndex + 2] = verts[2];
						}

						// Fill indices
						exportMesh->numIndices = mesh->GetPolygonVertexCount();
						exportMesh->indices = new int[exportMesh->numIndices];
						for (int j = 0; j < exportMesh->numIndices; ++j)
						{
							int index = mesh->GetPolygonVertices()[j];
							exportMesh->indices[j] = index;
							FbxVector4 vertex = mesh->GetControlPoints()[index];
							printf("Index %d has vertex   %.1f,  %.1f,  %.1f,   \n", index, vertex.mData[0], vertex.mData[1], vertex.mData[2]);
						}


						/* Test data
						int controlPointCount = mesh->GetControlPointsCount();
						FbxVector4 cpoints[8];
						for (unsigned j = 0; j < controlPointCount; ++j)
						{
							cpoints[j] = mesh->GetControlPointAt(j);
						}

						int polygonCount = mesh->GetPolygonCount();
						for (int poly = 0; poly < polygonCount; ++poly)
						{
							int polySize = mesh->GetPolygonSize(poly);
							for (int polyVertex = 0; polyVertex < polySize; ++polyVertex)
							{
								int index = mesh->GetPolygonVertex(poly, polyVertex);
								FbxVector4 v = cpoints[index];
								printf("Index %d has %.1f, %.1f, %.1f, \n", index, v.mData[0], v.mData[1], v.mData[2]);
							}
						}
						*/
					}
					break;
				}
			}
		}
	}

	printf("Mesh Stats: \n \n");
	printf("Total vertices: %d \n", exportMesh->numVertices);
	printf("Total indices: %d \n", exportMesh->numIndices);
	printf("Size: %d bytes \n", ((sizeof(float) * 3) * exportMesh->numVertices) + (sizeof(int) * exportMesh->numIndices));
	printf("Exporting to e3m format, please wait... \n");
	fileNameRaw.append(".e3m");

	{ // Write
		FILE* file;
		file = fopen(fileNameRaw.c_str(), "wb");
		// Write the number of vertices
		fwrite(&exportMesh->numVertices, sizeof(int), 1, file);
		// Write vertices
		fwrite(exportMesh->vertices, sizeof(float), exportMesh->numVertices * 3, file);
		// Write number of indices
		fwrite(&exportMesh->numIndices, sizeof(int), 1, file);
		// Write indices
		fwrite(exportMesh->indices, sizeof(int), exportMesh->numIndices, file);
		fclose(file);
	}

	/* This is how you read
	{ // Read
		Mesh* importMesh = new Mesh();

		FILE* file;
		file = fopen(fileNameRaw.c_str(), "rb");

		// Read vertices
		fread(&importMesh->numVertices, sizeof(int), 1, file);
		importMesh->vertices = new float[importMesh->numVertices * 3];
		fread(importMesh->vertices, sizeof(float), importMesh->numVertices * 3, file);

		// Read indices
		fread(&importMesh->numIndices, sizeof(int), 1, file);
		importMesh->indices = new int[importMesh->numIndices];
		fread(importMesh->indices, sizeof(int), importMesh->numIndices, file);

		fclose(file);
		
		// Check data
		float verts[8 * 3];
		for (int i = 0; i < 24; ++i)
			verts[i] = importMesh->vertices[i];
		float idx[24];
		for (int i = 0; i < 24; ++i)
			idx[i] = importMesh->indices[i];

		// Get rid of mesh
		delete importMesh;
	}*/

	printf("...Finished: Cleaning up. \n");
	delete exportMesh;
	manager->Destroy();
	return 0;
}
