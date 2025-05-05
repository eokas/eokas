#include "./FBXImporter.h"
#include "fbxsdk.h"
#include "mesh/RawMesh.h"

namespace eokas
{
#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(mManager->GetIOSettings()))
#endif
    
    struct FBXImporterImpl
    {
        FbxManager* mManager = nullptr;
        FbxScene* mScene = nullptr;
        
        bool init()
        {
            mManager = FbxManager::Create();
            if (mManager == nullptr)
            {
                FBXSDK_printf("Error: Unable to create FBX Manager!\n");
                return false;
            }
            else
            {
                FBXSDK_printf("Autodesk FBX SDK version %s\n", mManager->GetVersion());
            }
            
            //Create an IOSettings object. This object holds all import/export settings.
            FbxIOSettings* ios = FbxIOSettings::Create(mManager, IOSROOT);
            mManager->SetIOSettings(ios);

#ifndef FBXSDK_ENV_WINSTORE
            //Load plugins from the executable directory (optional)
            FbxString lPath = FbxGetApplicationDirectory();
            mManager->LoadPluginsDirectory(lPath.Buffer());
#endif
            
            //Create an FBX scene. This object holds most objects imported/exported from/to files.
            mScene = FbxScene::Create(mManager, "My Scene");
            if (mScene == nullptr)
            {
                FBXSDK_printf("Error: Unable to create FBX scene!\n");
                return false;
            }
            
            return true;
        }
        
        void quit()
        {
            if(mScene != nullptr)
            {
                mScene->Destroy();
            }
            
            if (mManager != nullptr)
            {
                mManager->Destroy();
            }
        }
        
        bool load(const char* filePath)
        {
            char password[1024];
            int animStackCount;
            
            // Get the file version number generate by the FBX SDK.
            int sdkMajor, sdkMinor, sdkRevision;
            FbxManager::GetFileFormatVersion(sdkMajor, sdkMinor, sdkRevision);
            
            // Create an importer.
            FbxImporter* fbxImporter = FbxImporter::Create(mManager, "");
            
            // Initialize the importer by providing a filename.
            const bool importResult = fbxImporter->Initialize(filePath, -1, mManager->GetIOSettings());
            
            int fileMajor, fileMinor, fileRevision;
            fbxImporter->GetFileVersion(fileMajor, fileMinor, fileRevision);
            
            if (!importResult)
            {
                FbxString error = fbxImporter->GetStatus().GetErrorString();
                FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
                FBXSDK_printf("Error returned: %s\n\n", error.Buffer());
                
                if (fbxImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
                {
                    FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", sdkMajor, sdkMinor, sdkRevision);
                    FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", filePath, fileMajor, fileMinor, fileRevision);
                }
                
                return false;
            }
            
            FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", sdkMajor, sdkMinor, sdkRevision);
            
            if (fbxImporter->IsFBX())
            {
                FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", filePath, fileMajor, fileMinor, fileRevision);
                
                // From this point, it is possible to access animation stack information without
                // the expense of loading the entire file.
                
                FBXSDK_printf("Animation Stack Information\n");
                
                animStackCount = fbxImporter->GetAnimStackCount();
                
                FBXSDK_printf("    Number of Animation Stacks: %d\n", animStackCount);
                FBXSDK_printf("    Current Animation Stack: \"%s\"\n", fbxImporter->GetActiveAnimStackName().Buffer());
                FBXSDK_printf("\n");
                
                for (int i = 0; i < animStackCount; i++)
                {
                    FbxTakeInfo* lTakeInfo = fbxImporter->GetTakeInfo(i);
                    
                    FBXSDK_printf("    Animation Stack %d\n", i);
                    FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
                    FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());
                    
                    // Change the value of the import name if the animation stack should be imported
                    // under a different name.
                    FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());
                    
                    // Set the value of the import state to false if the animation stack should be not
                    // be imported.
                    FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
                    FBXSDK_printf("\n");
                }
                
                // Set the import states. By default, the import states are always set to
                // true. The code below shows how to change these states.
                IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
                IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
                IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
                IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
                IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
                IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
                IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
            }
            
            // Import the scene.
            bool result = fbxImporter->Import(mScene);
            if (!result && fbxImporter->GetStatus() == FbxStatus::ePasswordError)
            {
                FBXSDK_printf("Please enter password: ");
                
                password[0] = '\0';
                
                FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
                scanf("%s", password);
                FBXSDK_CRT_SECURE_NO_WARNING_END
                
                FbxString lString(password);
                
                IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
                IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);
                
                result = fbxImporter->Import(mScene);
                if (!result && fbxImporter->GetStatus() == FbxStatus::ePasswordError)
                {
                    FBXSDK_printf("\nPassword is wrong, import aborted.\n");
                }
            }
            
            if (!result || (fbxImporter->GetStatus() != FbxStatus::eSuccess))
            {
                FBXSDK_printf("********************************************************************************\n");
                if (result)
                {
                    FBXSDK_printf("WARNING:\n");
                    FBXSDK_printf("   The importer was able to read the file but with errors.\n");
                    FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
                }
                else
                {
                    FBXSDK_printf("Importer failed to load the file!\n\n");
                }
                
                if (fbxImporter->GetStatus() != FbxStatus::eSuccess)
                    FBXSDK_printf("   Last error message: %s\n", fbxImporter->GetStatus().GetErrorString());
                
                FbxArray<FbxString*> history;
                fbxImporter->GetStatus().GetErrorStringHistory(history);
                if (history.GetCount() > 1)
                {
                    FBXSDK_printf("   Error history stack:\n");
                    for (int i = 0; i < history.GetCount(); i++)
                    {
                        FBXSDK_printf("      %s\n", history[i]->Buffer());
                    }
                }
                FbxArrayDelete<FbxString*>(history);
                FBXSDK_printf("********************************************************************************\n");
            }
            
            // Destroy the importer.
            fbxImporter->Destroy();
            
            if(result)
            {
                FbxGeometryConverter geometryConverter(mManager);
                geometryConverter.Triangulate(mScene, true);
            }
            
            return result;
        }
        
        bool save(FbxDocument* scene, const char* filePath, int fileFormat, bool embedMedia)
        {
            int lMajor, lMinor, lRevision;
            bool lStatus = true;
            
            // Create an exporter.
            FbxExporter* fbxExporter = FbxExporter::Create(mManager, "");
            
            if (fileFormat < 0 || fileFormat >= mManager->GetIOPluginRegistry()->GetWriterFormatCount())
            {
                // Write in fall back format in less no ASCII format found
                fileFormat = mManager->GetIOPluginRegistry()->GetNativeWriterFormat();
                
                //Try to export in ASCII if possible
                int formatCount = mManager->GetIOPluginRegistry()->GetWriterFormatCount();
                for (int formatIndex = 0; formatIndex < formatCount; formatIndex++)
                {
                    if (mManager->GetIOPluginRegistry()->WriterIsFBX(formatIndex))
                    {
                        FbxString lDesc = mManager->GetIOPluginRegistry()->GetWriterFormatDescription(formatIndex);
                        const char* lASCII = "ascii";
                        if (lDesc.Find(lASCII) >= 0)
                        {
                            fileFormat = formatIndex;
                            break;
                        }
                    }
                }
            }
            
            // Set the export states. By default, the export states are always set to
            // true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below
            // shows how to change these states.
            IOS_REF.SetBoolProp(EXP_FBX_MATERIAL, true);
            IOS_REF.SetBoolProp(EXP_FBX_TEXTURE, true);
            IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED, embedMedia);
            IOS_REF.SetBoolProp(EXP_FBX_SHAPE, true);
            IOS_REF.SetBoolProp(EXP_FBX_GOBO, true);
            IOS_REF.SetBoolProp(EXP_FBX_ANIMATION, true);
            IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
            
            // Initialize the exporter by providing a filename.
            if (fbxExporter->Initialize(filePath, fileFormat, mManager->GetIOSettings()) == false)
            {
                FBXSDK_printf("Call to FbxExporter::Initialize() failed.\n");
                FBXSDK_printf("Error returned: %s\n\n", fbxExporter->GetStatus().GetErrorString());
                return false;
            }
            
            FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
            FBXSDK_printf("FBX file format version %d.%d.%d\n\n", lMajor, lMinor, lRevision);
            
            // Export the scene.
            bool result = fbxExporter->Export(scene);
            
            // Destroy the exporter.
            fbxExporter->Destroy();
            
            return result;
        }
        
        bool fill(RawMesh& mesh)
        {
            if (mScene == nullptr)
                return false;
            
            mesh.clear();
            
            FbxNode* rootNode = mScene->GetRootNode();
            if (!rootNode)
            {
                throw std::runtime_error("FBX scene has no root node");
            }
            
            bool meshImported = false;
            for (int i = 0; i < rootNode->GetChildCount(); ++i)
            {
                FbxNode* node = rootNode->GetChild(i);
                FbxMesh* fbxMesh = node->GetMesh();
                
                if (fbxMesh)
                {
                    mesh.clear();
                    meshImported = true;
                    
                    // 处理顶点数据
                    const FbxVector4* vertices = fbxMesh->GetControlPoints();
                    const int vertexCount = fbxMesh->GetControlPointsCount();
                    std::vector<VertexID> vertexIds;
                    vertexIds.reserve(vertexCount);
                    
                    for (int v = 0; v < vertexCount; ++v)
                    {
                        RawMesh::Vertex vertex;
                        vertex.pos = {static_cast<float>(vertices[v][0]), static_cast<float>(vertices[v][1]), static_cast<float>(vertices[v][2])};
                        vertexIds.push_back(mesh.addVertex(vertex));
                    }
                    
                    // 处理多边形数据
                    const int polygonCount = fbxMesh->GetPolygonCount();
                    std::vector<CornerID> cornerIds;
                    cornerIds.reserve(polygonCount * 3);
                    
                    for (int p = 0; p < polygonCount; ++p)
                    {
                        for (int v = 0; v < 3; ++v)
                        { // 已经三角化
                            const int cpIndex = fbxMesh->GetPolygonVertex(p, v);
                            if (cpIndex < 0 || cpIndex >= vertexCount)
                            {
                                continue; // 跳过无效索引
                            }
                            cornerIds.push_back(mesh.addCorner(vertexIds[cpIndex]));
                        }
                    }
                    
                    // 创建三角形
                    for (int t = 0; t < polygonCount; ++t)
                    {
                        if (t * 3 + 2 >= cornerIds.size()) break;
                        
                        mesh.addTriangle({cornerIds[t * 3], cornerIds[t * 3 + 1], cornerIds[t * 3 + 2]});
                    }
                    
                    // 处理UV数据
                    FbxStringList uvSetNames;
                    fbxMesh->GetUVSetNames(uvSetNames);
                    for (int uvSetIndex = 0; uvSetIndex < uvSetNames.GetCount(); ++uvSetIndex)
                    {
                        const char* uvSetName = uvSetNames.GetStringAt(uvSetIndex);
                        const FbxGeometryElementUV* uvElement = fbxMesh->GetElementUV(uvSetName);
                        if (uvElement == nullptr)
                            continue;
                        
                        // 创建UV属性
                        const auto uvAttrId = mesh.addAttribute<Vector2>(RawMesh::AttributeUsage::UV);
                        auto& uvAttr = mesh.getAttribute<Vector2>(uvAttrId);
                        uvAttr.ensure(cornerIds.size());
                        
                        // 获取映射模式
                        const auto mappingMode = uvElement->GetMappingMode();
                        const auto referenceMode = uvElement->GetReferenceMode();
                        const bool useIndex = (referenceMode == FbxGeometryElement::eIndexToDirect);
                        
                        // 1. 首先建立corner到vertex的映射
                        std::vector<VertexID> cornerToVertex(cornerIds.size());
                        for (int c = 0; c < cornerIds.size(); ++c)
                        {
                            cornerToVertex[c] = mesh.getCorner(cornerIds[c]).vertexId;
                        }
                        
                        // 2. 根据映射模式处理UV
                        if (mappingMode == FbxLayerElement::eByPolygonVertex)
                        {
                            // 直接按角点顺序映射
                            for (int c = 0; c < cornerIds.size(); ++c)
                            {
                                const int uvIndex = useIndex ? uvElement->GetIndexArray().GetAt(c) : c;
                                
                                if (uvIndex >= 0 && uvIndex < uvElement->GetDirectArray().GetCount())
                                {
                                    const FbxVector2& uv = uvElement->GetDirectArray().GetAt(uvIndex);
                                    uvAttr.set(cornerIds[c], {static_cast<float>(uv[0]), static_cast<float>(uv[1])});
                                }
                            }
                        }
                        else if (mappingMode == FbxLayerElement::eByControlPoint)
                        {
                            // 通过顶点间接映射
                            for (int c = 0; c < cornerIds.size(); ++c)
                            {
                                const int vertexIndex = cornerToVertex[c];
                                const int uvIndex = useIndex ? uvElement->GetIndexArray().GetAt(vertexIndex) : vertexIndex;
                                
                                if (uvIndex >= 0 && uvIndex < uvElement->GetDirectArray().GetCount())
                                {
                                    const FbxVector2& uv = uvElement->GetDirectArray().GetAt(uvIndex);
                                    uvAttr.set(cornerIds[c], {static_cast<float>(uv[0]), static_cast<float>(uv[1])});
                                }
                            }
                        }
                        else if (mappingMode == FbxLayerElement::eByPolygon)
                        {
                            // 按多边形映射（所有角点共享相同UV）
                            const int trianglesCount = mesh.getTriangleCount();
                            for (int t = 0; t < trianglesCount; ++t)
                            {
                                const int uvIndex = useIndex ? uvElement->GetIndexArray().GetAt(t) : t;
                                
                                if (uvIndex >= 0 && uvIndex < uvElement->GetDirectArray().GetCount())
                                {
                                    const FbxVector2& uv = uvElement->GetDirectArray().GetAt(uvIndex);
                                    const auto& triangle = mesh.getTriangle(t);
                                    uvAttr.set(triangle.c0, {static_cast<float>(uv[0]), static_cast<float>(uv[1])});
                                    uvAttr.set(triangle.c1, {static_cast<float>(uv[0]), static_cast<float>(uv[1])});
                                    uvAttr.set(triangle.c2, {static_cast<float>(uv[0]), static_cast<float>(uv[1])});
                                }
                            }
                        }
                    }
                    
                    // 处理法线数据
                    for (int layerIndex = 0; layerIndex < fbxMesh->GetLayerCount(); ++layerIndex)
                    {
                        FbxLayer* layer = fbxMesh->GetLayer(layerIndex);
                        if (!layer) continue;
                        
                        FbxLayerElementNormal* normalLayer = layer->GetNormals();
                        if (!normalLayer) continue;
                        
                        const auto normalAttrId = mesh.addAttribute<Vector3>(RawMesh::AttributeUsage::Normal);
                        auto& normalAttr = mesh.getAttribute<Vector3>(normalAttrId);
                        normalAttr.ensure(cornerIds.size());
                        
                        const auto mappingMode = normalLayer->GetMappingMode();
                        const auto refMode = normalLayer->GetReferenceMode();
                        
                        if (mappingMode == FbxLayerElement::eByPolygonVertex)
                        {
                            for (int c = 0; c < cornerIds.size(); ++c)
                            {
                                int normalIndex = (refMode == FbxLayerElement::eDirect) ? c : normalLayer->GetIndexArray().GetAt(c);
                                
                                if (normalIndex >= 0 && normalIndex < normalLayer->GetDirectArray().GetCount())
                                {
                                    const FbxVector4& normal = normalLayer->GetDirectArray().GetAt(normalIndex);
                                    normalAttr.set(cornerIds[c], {static_cast<float>(normal[0]), static_cast<float>(normal[1]), static_cast<float>(normal[2])});
                                }
                            }
                        }
                    }
                    
                    // 处理骨骼权重
                    for (int deformerIndex = 0; deformerIndex < fbxMesh->GetDeformerCount(); ++deformerIndex)
                    {
                        FbxSkin* skin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
                        if (!skin) continue;
                        
                        std::vector<RawMesh::Weight> weights(vertexCount);
                        
                        for (int clusterIndex = 0; clusterIndex < skin->GetClusterCount(); ++clusterIndex)
                        {
                            FbxCluster* cluster = skin->GetCluster(clusterIndex);
                            if (!cluster) continue;
                            
                            const int* vertexIndices = cluster->GetControlPointIndices();
                            const double* vertexWeights = cluster->GetControlPointWeights();
                            const int vertexIndexCount = cluster->GetControlPointIndicesCount();
                            
                            for (int v = 0; v < vertexIndexCount; ++v)
                            {
                                const int vertexIndex = vertexIndices[v];
                                if (vertexIndex < 0 || vertexIndex >= vertexCount) continue;
                                
                                const float weight = static_cast<float>(vertexWeights[v]);
                                if (weight <= 0.0f) continue;
                                
                                // 找到最小权重的插槽
                                int minSlot = 0;
                                for (int s = 1; s < RawMesh::VERTEX_BONE_WEIGHT_COUNT; ++s)
                                {
                                    if (weights[vertexIndex].weights[s] < weights[vertexIndex].weights[minSlot])
                                    {
                                        minSlot = s;
                                    }
                                }
                                
                                // 替换最小权重
                                if (weight > weights[vertexIndex].weights[minSlot])
                                {
                                    weights[vertexIndex].bones[minSlot] = clusterIndex;
                                    weights[vertexIndex].weights[minSlot] = weight;
                                }
                            }
                        }
                        
                        // 归一化并设置权重
                        for (int v = 0; v < vertexCount; ++v)
                        {
                            weights[v].normalize();
                            mesh.setWeight(vertexIds[v], weights[v]);
                        }
                    }
                    
                    // 创建默认Section
                    SectionID sectionId = mesh.addSection();
                    for (int t = 0; t < polygonCount; ++t)
                    {
                        mesh.addSectionTriangle(sectionId, t);
                    }
                    
                    break; // 只导入第一个找到的网格
                }
            }
        }
    };
    
    FBXImporter::FBXImporter()
        : mImpl(new FBXImporterImpl())
    { }
    
    FBXImporter::~FBXImporter()
    {
        _DeletePointer(mImpl)
    }
    
    bool FBXImporter::init()
    {
        return mImpl->init();
    }
    
    void FBXImporter::quit()
    {
        return mImpl->quit();
    }
}