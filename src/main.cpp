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
	float* vertexColors;
	int numIndices;
	int* indices;
};

int main(int argc, char *argv[])
{
	// The mesh instance being exported
	Mesh* exportMesh;
	exportMesh = new Mesh();

	std::string fileName = "cube.fbx";
	/*if (argv[1] == "-f")
	{
		fileName = argv[2];
	}
	else
	{
		printf("Usage: \n");
		printf("-f <filename> \n");
		return 0;
	}*/
	
	printf("Importing %s \n", fileName.c_str());
	
	// Strip extension and store raw file name for future reference
	std::string fileNameRaw = fileName.substr(0, fileName.find_last_of("."));

	// Initialize fbx sdk
	FbxManager* fbxManager = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
	FbxScene* scene = FbxScene::Create(fbxManager, "Scene");

	// Get sdk version info
	int sdkMajor, sdkMinor, sdkRev;
	FbxManager::GetFileFormatVersion(sdkMajor, sdkMinor, sdkRev);

	FbxImporter* importer = FbxImporter::Create(fbxManager, "");
	bool status = importer->Initialize(fileName.c_str(), -1, fbxManager->GetIOSettings());

	if (!status)
	{
		// Print error
		FbxString error = importer->GetStatus().GetErrorString();
		printf("Failed to initialize fbx importer. \n");
		printf("Error: %s \n", error.Buffer());
		return 0;
	}

	// Get file version info
	int fileMajor, fileMinor, fileRev;
	importer->GetFileVersion(fileMajor, fileMinor, fileRev);

	printf("FBX SDK version: %d.%d.%d \n", sdkMajor, sdkMinor, sdkRev);
	if (importer->IsFBX())
	{
		printf("FBX file format version: %d.%d.%d \n", fileMajor, fileMinor, fileRev);

	}
	else
	{
		printf("File format is not supported. \n");
		return 0;
	}

	// Import scene
	status = importer->Import(scene);

	// Triangulate scene
	// NOTE: We assume that the scene vertices are not already triangulated
	FbxGeometryConverter* geometryConverter;
	geometryConverter = new FbxGeometryConverter(fbxManager);
	geometryConverter->Triangulate(scene, true);

	// Traverse node
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

						// Grab vertices
						int triCount = mesh->GetPolygonCount();
						int currentVert = 0;
						exportMesh->numVertices = triCount * 3;
						exportMesh->vertices = new float[exportMesh->numVertices * 3];
						for (int tri = 0; tri < triCount; ++tri)
						{
							for (int vertex = 0; vertex < 3; ++vertex)
							{
								int index = mesh->GetPolygonVertex(tri, vertex);
								FbxVector4 v = mesh->GetControlPoints()[index];
								auto c = mesh->GetElementVertexColor(index);
								
								printf("Vertex %d: %.1f, %.1f, %.1f, \n", currentVert, v.mData[0], v.mData[1], v.mData[2]);
								
								int dstIndex = currentVert * 3;
								exportMesh->vertices[dstIndex + 0] = static_cast<float>(v.mData[0]);
								exportMesh->vertices[dstIndex + 1] = static_cast<float>(v.mData[1]);
								exportMesh->vertices[dstIndex + 2] = static_cast<float>(v.mData[2]);

								++currentVert;
							}
						}

						// Note: Mesh indices was supported in an older format
						// Add some dumb values for now
						exportMesh->numIndices = 1;
						exportMesh->indices = new int[1];
						exportMesh->indices[0] = -1;

						// Grab vertex colors
						// TODO: Add vertex color support in e3m format
						// TODO: Add support for multiple color layers
						
						// Example code
						exportMesh->vertexColors = new float[exportMesh->numVertices * 4];
						for (int i = 0; i < exportMesh->numVertices; ++i)
						{
							int colorIndex = mesh->GetLayer(0)->GetVertexColors()->GetIndexArray()[i];
							FbxColor color = mesh->GetLayer(0)->GetVertexColors()->GetDirectArray()[colorIndex];

							int dstIndex = i * 4;
							exportMesh->vertexColors[dstIndex + 0] = color.mRed;
							exportMesh->vertexColors[dstIndex + 1] = color.mGreen;
							exportMesh->vertexColors[dstIndex + 2] = color.mBlue;
							exportMesh->vertexColors[dstIndex + 3] = color.mAlpha;
						}
						
					}
					break;
				}
			}
		}
	}

	// Export mesh
	unsigned sizeOfVertices = sizeof(float) * 3 * exportMesh->numVertices;
	unsigned sizeOfIndices = sizeof(int) * exportMesh->numIndices;

	printf("Mesh Stats: \n \n");
	printf("Total vertices: %d \n", exportMesh->numVertices);
	printf("Total indices: %d \n", exportMesh->numIndices);
	printf("Size: %d bytes \n", sizeOfVertices + sizeOfIndices + sizeof(exportMesh->numIndices) + sizeof(exportMesh->numVertices));
	printf("Exporting to e3m format, please wait... \n");
	
	std::string exportFileName = fileNameRaw.append(".e3m");

	{ // Write
		FILE* file;
		file = fopen(exportFileName.c_str(), "wb");
		// Write the number of vertices
		fwrite(&exportMesh->numVertices, sizeof(int), 1, file);
		// Write vertices
		fwrite(exportMesh->vertices, sizeof(float), exportMesh->numVertices * 3, file);
		// Write vertex colors
		fwrite(exportMesh->vertexColors, sizeof(float), exportMesh->numVertices * 4, file);
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
		file = fopen(exportFileName.c_str(), "rb");

		// Read vertices
		fread(&importMesh->numVertices, sizeof(int), 1, file);
		importMesh->vertices = new float[importMesh->numVertices * 3];
		fread(importMesh->vertices, sizeof(float), importMesh->numVertices * 3, file);

		// Read vertex colors
		importMesh->vertexColors = new float[importMesh->numVertices * 4];
		fread(importMesh->vertexColors, sizeof(float), importMesh->numVertices * 4, file);

		// Read indices
		fread(&importMesh->numIndices, sizeof(int), 1, file);
		importMesh->indices = new int[importMesh->numIndices];
		fread(importMesh->indices, sizeof(int), importMesh->numIndices, file);

		fclose(file);

		// Get rid of mesh
		delete importMesh;
	}*/
	
	printf("Finished: Cleaning up. \n");
	delete exportMesh;
	fbxManager->Destroy();
	return 0;
}
