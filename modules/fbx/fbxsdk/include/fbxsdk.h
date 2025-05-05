/****************************************************************************************
 
   Copyright (C) 2016 Autodesk, Inc.
   All rights reserved.
 
   Use of this software is subject to the terms of the Autodesk license agreement
   provided at the time of installation or download, or which otherwise accompanies
   this software in either electronic or hard copy form.
 
****************************************************************************************/

//! \file fbxsdk.h
#ifndef _FBXSDK_H_
#define _FBXSDK_H_

/**
  * \mainpage FBX SDK Reference
  * <p>
  * \section welcome Welcome to the FBX SDK Reference
  * The FBX SDK Reference contains reference information on every header file, 
  * namespace, class, method, enum, typedef, variable, and other C++ elements 
  * that comprise the FBX software development kit (SDK).
  * <p>
  * The FBX SDK Reference is organized into the following sections:
  * <ul><li>Class List: an alphabetical list of FBX SDK classes
  *     <li>Class Hierarchy: a textual representation of the FBX SDK class structure
  *     <li>Graphical Class Hierarchy: a graphical representation of the FBX SDK class structure
  *     <li>File List: an alphabetical list of all documented header files</ul>
  * <p>
  * \section otherdocumentation Other Documentation
  * Apart from this reference guide, an FBX SDK Programming Guide and many FBX 
  * SDK examples are also provided.
  * <p>
  * \section aboutFBXSDK About the FBX SDK
  * The FBX SDK is a C++ software development kit (SDK) that lets you import 
  * and export 3D scenes using the Autodesk FBX file format. The FBX SDK 
  * reads FBX files created with FiLMBOX version 2.5 and later and writes FBX 
  * files compatible with MotionBuilder version 6.0 and up. 
  */

#include "fbx/fbxsdk/include/fbxsdk/fbxsdk_def.h"

#ifndef FBXSDK_NAMESPACE_USING
	#define FBXSDK_NAMESPACE_USING 1
#endif

//---------------------------------------------------------------------------------------
//Core Base Includes
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxarray.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxbitset.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxcharptrset.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxcontainerallocators.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxdynamicarray.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxstatus.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxfile.h"
#ifndef FBXSDK_ENV_WINSTORE
	#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxfolder.h"
#endif
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxhashmap.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxintrusivelist.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxmap.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxmemorypool.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxpair.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxset.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxstring.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxstringlist.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxtime.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxtimecode.h"
#include "fbx/fbxsdk/include/fbxsdk/core/base/fbxutils.h"

//---------------------------------------------------------------------------------------
//Core Math Includes
#include "fbx/fbxsdk/include/fbxsdk/core/math/fbxmath.h"
#include "fbx/fbxsdk/include/fbxsdk/core/math/fbxdualquaternion.h"
#include "fbx/fbxsdk/include/fbxsdk/core/math/fbxmatrix.h"
#include "fbx/fbxsdk/include/fbxsdk/core/math/fbxquaternion.h"
#include "fbx/fbxsdk/include/fbxsdk/core/math/fbxvector2.h"
#include "fbx/fbxsdk/include/fbxsdk/core/math/fbxvector4.h"

//---------------------------------------------------------------------------------------
//Core Sync Includes
#ifndef FBXSDK_ENV_WINSTORE
	#include "fbx/fbxsdk/include/fbxsdk/core/sync/fbxatomic.h"
	#include "fbx/fbxsdk/include/fbxsdk/core/sync/fbxclock.h"
	#include "fbx/fbxsdk/include/fbxsdk/core/sync/fbxsync.h"
	#include "fbx/fbxsdk/include/fbxsdk/core/sync/fbxthread.h"
#endif /* !FBXSDK_ENV_WINSTORE */

//---------------------------------------------------------------------------------------
//Core Includes
#include "fbx/fbxsdk/include/fbxsdk/core/fbxclassid.h"
#include "fbx/fbxsdk/include/fbxsdk/core/fbxconnectionpoint.h"
#include "fbx/fbxsdk/include/fbxsdk/core/fbxdatatypes.h"
#ifndef FBXSDK_ENV_WINSTORE
	#include "fbx/fbxsdk/include/fbxsdk/core/fbxmodule.h"
	#include "fbx/fbxsdk/include/fbxsdk/core/fbxloadingstrategy.h"
#endif /* !FBXSDK_ENV_WINSTORE */
#include "fbx/fbxsdk/include/fbxsdk/core/fbxmanager.h"
#include "fbx/fbxsdk/include/fbxsdk/core/fbxobject.h"
#include "fbx/fbxsdk/include/fbxsdk/core/fbxperipheral.h"
#ifndef FBXSDK_ENV_WINSTORE
	#include "fbx/fbxsdk/include/fbxsdk/core/fbxplugin.h"
	#include "fbx/fbxsdk/include/fbxsdk/core/fbxplugincontainer.h"
#endif /* !FBXSDK_ENV_WINSTORE */
#include "fbx/fbxsdk/include/fbxsdk/core/fbxproperty.h"
#include "fbx/fbxsdk/include/fbxsdk/core/fbxpropertydef.h"
#include "fbx/fbxsdk/include/fbxsdk/core/fbxpropertyhandle.h"
#include "fbx/fbxsdk/include/fbxsdk/core/fbxpropertypage.h"
#include "fbx/fbxsdk/include/fbxsdk/core/fbxpropertytypes.h"
#include "fbx/fbxsdk/include/fbxsdk/core/fbxquery.h"
#include "fbx/fbxsdk/include/fbxsdk/core/fbxqueryevent.h"
#ifndef FBXSDK_ENV_WINSTORE
	#include "fbx/fbxsdk/include/fbxsdk/core/fbxscopedloadingdirectory.h"
	#include "fbx/fbxsdk/include/fbxsdk/core/fbxscopedloadingfilename.h"
#endif /* !FBXSDK_ENV_WINSTORE */
#include "fbx/fbxsdk/include/fbxsdk/core/fbxxref.h"

//---------------------------------------------------------------------------------------
//File I/O Includes
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxexporter.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxexternaldocreflistener.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxfiletokens.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxglobalcamerasettings.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxgloballightsettings.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxgobo.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbximporter.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxiobase.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxiopluginregistry.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxiosettings.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxstatisticsfbx.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxstatistics.h"
#include "fbx/fbxsdk/include/fbxsdk/fileio/fbxcallbacks.h"

//---------------------------------------------------------------------------------------
//Scene Includes
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxaudio.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxaudiolayer.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxcollection.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxcollectionexclusive.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxcontainer.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxcontainertemplate.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxdisplaylayer.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxdocument.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxdocumentinfo.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxenvironment.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxgroupname.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxlibrary.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxmediaclip.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxobjectmetadata.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxpose.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxreference.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxscene.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxselectionset.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxselectionnode.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxtakeinfo.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxthumbnail.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/fbxvideo.h"

//---------------------------------------------------------------------------------------
//Scene Animation Includes
#include "fbx/fbxsdk/include/fbxsdk/scene/animation/fbxanimcurve.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/animation/fbxanimcurvebase.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/animation/fbxanimcurvefilters.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/animation/fbxanimcurvenode.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/animation/fbxanimevalclassic.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/animation/fbxanimevalstate.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/animation/fbxanimevaluator.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/animation/fbxanimlayer.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/animation/fbxanimstack.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/animation/fbxanimutilities.h"

//---------------------------------------------------------------------------------------
//Scene Constraint Includes
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxcharacternodename.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxcharacter.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxcharacterpose.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraint.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintaim.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintcustom.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintparent.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintposition.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintrotation.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintscale.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintsinglechainik.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintutils.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxcontrolset.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/constraint/fbxhik2fbxcharacter.h"

//---------------------------------------------------------------------------------------
//Scene Geometry Includes
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxblendshape.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxblendshapechannel.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxcache.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxcachedeffect.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxcamera.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxcamerastereo.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxcameraswitcher.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxcluster.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxdeformer.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxgenericnode.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxgeometry.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxgeometrybase.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxgeometryweightedmap.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxlight.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxlimitsutilities.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxline.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxlodgroup.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxmarker.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxmesh.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxnode.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxnodeattribute.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxnull.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxnurbs.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxnurbscurve.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxnurbssurface.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxopticalreference.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxpatch.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxproceduralgeometry.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxshape.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxskeleton.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxskin.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxsubdeformer.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxsubdiv.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxtrimnurbssurface.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxvertexcachedeformer.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/geometry/fbxweightedmapping.h"

//---------------------------------------------------------------------------------------
//Scene Shading Includes
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxshadingconventions.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxbindingsentryview.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxbindingtable.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxbindingtableentry.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxbindingoperator.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxconstantentryview.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxentryview.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxfiletexture.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbximplementation.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbximplementationfilter.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbximplementationutils.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxlayeredtexture.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxoperatorentryview.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxproceduraltexture.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxpropertyentryview.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxsemanticentryview.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxsurfacelambert.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxsurfacematerial.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxsurfacematerialutils.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxsurfacephong.h"
#include "fbx/fbxsdk/include/fbxsdk/scene/shading/fbxtexture.h"

//---------------------------------------------------------------------------------------
//Utilities Includes
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxdeformationsevaluator.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxprocessor.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxprocessorxref.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxprocessorxrefuserlib.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxprocessorshaderdependency.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxclonemanager.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxgeometryconverter.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxmanipulators.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxmaterialconverter.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxrenamingstrategyfbx5.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxrenamingstrategyfbx6.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxrenamingstrategyutilities.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxrootnodeutility.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxusernotification.h"
#include "fbx/fbxsdk/include/fbxsdk/utils/fbxscenecheckutility.h"

//---------------------------------------------------------------------------------------
#if defined(FBXSDK_NAMESPACE) && (FBXSDK_NAMESPACE_USING == 1)
	using namespace FBXSDK_NAMESPACE;
#endif

#endif /* _FBXSDK_H_ */
