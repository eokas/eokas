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

#include "FBX/fbxsdk/include/fbxsdk/fbxsdk_def.h"

#ifndef FBXSDK_NAMESPACE_USING
	#define FBXSDK_NAMESPACE_USING 1
#endif

//---------------------------------------------------------------------------------------
//Core Base Includes
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxarray.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxbitset.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxcharptrset.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxcontainerallocators.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxdynamicarray.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxstatus.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxfile.h"
#ifndef FBXSDK_ENV_WINSTORE
	#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxfolder.h"
#endif
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxhashmap.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxintrusivelist.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxmap.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxmemorypool.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxpair.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxset.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxstring.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxstringlist.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxtime.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxtimecode.h"
#include "FBX/fbxsdk/include/fbxsdk/core/base/fbxutils.h"

//---------------------------------------------------------------------------------------
//Core Math Includes
#include "FBX/fbxsdk/include/fbxsdk/core/math/fbxmath.h"
#include "FBX/fbxsdk/include/fbxsdk/core/math/fbxdualquaternion.h"
#include "FBX/fbxsdk/include/fbxsdk/core/math/fbxmatrix.h"
#include "FBX/fbxsdk/include/fbxsdk/core/math/fbxquaternion.h"
#include "FBX/fbxsdk/include/fbxsdk/core/math/fbxvector2.h"
#include "FBX/fbxsdk/include/fbxsdk/core/math/fbxvector4.h"

//---------------------------------------------------------------------------------------
//Core Sync Includes
#ifndef FBXSDK_ENV_WINSTORE
	#include "FBX/fbxsdk/include/fbxsdk/core/sync/fbxatomic.h"
	#include "FBX/fbxsdk/include/fbxsdk/core/sync/fbxclock.h"
	#include "FBX/fbxsdk/include/fbxsdk/core/sync/fbxsync.h"
	#include "FBX/fbxsdk/include/fbxsdk/core/sync/fbxthread.h"
#endif /* !FBXSDK_ENV_WINSTORE */

//---------------------------------------------------------------------------------------
//Core Includes
#include "FBX/fbxsdk/include/fbxsdk/core/fbxclassid.h"
#include "FBX/fbxsdk/include/fbxsdk/core/fbxconnectionpoint.h"
#include "FBX/fbxsdk/include/fbxsdk/core/fbxdatatypes.h"
#ifndef FBXSDK_ENV_WINSTORE
	#include "FBX/fbxsdk/include/fbxsdk/core/fbxmodule.h"
	#include "FBX/fbxsdk/include/fbxsdk/core/fbxloadingstrategy.h"
#endif /* !FBXSDK_ENV_WINSTORE */
#include "FBX/fbxsdk/include/fbxsdk/core/fbxmanager.h"
#include "FBX/fbxsdk/include/fbxsdk/core/fbxobject.h"
#include "FBX/fbxsdk/include/fbxsdk/core/fbxperipheral.h"
#ifndef FBXSDK_ENV_WINSTORE
	#include "FBX/fbxsdk/include/fbxsdk/core/fbxplugin.h"
	#include "FBX/fbxsdk/include/fbxsdk/core/fbxplugincontainer.h"
#endif /* !FBXSDK_ENV_WINSTORE */
#include "FBX/fbxsdk/include/fbxsdk/core/fbxproperty.h"
#include "FBX/fbxsdk/include/fbxsdk/core/fbxpropertydef.h"
#include "FBX/fbxsdk/include/fbxsdk/core/fbxpropertyhandle.h"
#include "FBX/fbxsdk/include/fbxsdk/core/fbxpropertypage.h"
#include "FBX/fbxsdk/include/fbxsdk/core/fbxpropertytypes.h"
#include "FBX/fbxsdk/include/fbxsdk/core/fbxquery.h"
#include "FBX/fbxsdk/include/fbxsdk/core/fbxqueryevent.h"
#ifndef FBXSDK_ENV_WINSTORE
	#include "FBX/fbxsdk/include/fbxsdk/core/fbxscopedloadingdirectory.h"
	#include "FBX/fbxsdk/include/fbxsdk/core/fbxscopedloadingfilename.h"
#endif /* !FBXSDK_ENV_WINSTORE */
#include "FBX/fbxsdk/include/fbxsdk/core/fbxxref.h"

//---------------------------------------------------------------------------------------
//File I/O Includes
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxexporter.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxexternaldocreflistener.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxfiletokens.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxglobalcamerasettings.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxgloballightsettings.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxgobo.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbximporter.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxiobase.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxiopluginregistry.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxiosettings.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxstatisticsfbx.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxstatistics.h"
#include "FBX/fbxsdk/include/fbxsdk/fileio/fbxcallbacks.h"

//---------------------------------------------------------------------------------------
//Scene Includes
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxaudio.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxaudiolayer.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxcollection.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxcollectionexclusive.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxcontainer.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxcontainertemplate.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxdisplaylayer.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxdocument.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxdocumentinfo.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxenvironment.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxgroupname.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxlibrary.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxmediaclip.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxobjectmetadata.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxpose.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxreference.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxscene.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxselectionset.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxselectionnode.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxtakeinfo.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxthumbnail.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/fbxvideo.h"

//---------------------------------------------------------------------------------------
//Scene Animation Includes
#include "FBX/fbxsdk/include/fbxsdk/scene/animation/fbxanimcurve.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/animation/fbxanimcurvebase.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/animation/fbxanimcurvefilters.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/animation/fbxanimcurvenode.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/animation/fbxanimevalclassic.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/animation/fbxanimevalstate.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/animation/fbxanimevaluator.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/animation/fbxanimlayer.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/animation/fbxanimstack.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/animation/fbxanimutilities.h"

//---------------------------------------------------------------------------------------
//Scene Constraint Includes
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxcharacternodename.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxcharacter.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxcharacterpose.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraint.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintaim.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintcustom.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintparent.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintposition.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintrotation.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintscale.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintsinglechainik.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxconstraintutils.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxcontrolset.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/constraint/fbxhik2fbxcharacter.h"

//---------------------------------------------------------------------------------------
//Scene Geometry Includes
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxblendshape.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxblendshapechannel.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxcache.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxcachedeffect.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxcamera.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxcamerastereo.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxcameraswitcher.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxcluster.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxdeformer.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxgenericnode.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxgeometry.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxgeometrybase.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxgeometryweightedmap.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxlight.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxlimitsutilities.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxline.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxlodgroup.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxmarker.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxmesh.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxnode.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxnodeattribute.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxnull.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxnurbs.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxnurbscurve.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxnurbssurface.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxopticalreference.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxpatch.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxproceduralgeometry.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxshape.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxskeleton.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxskin.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxsubdeformer.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxsubdiv.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxtrimnurbssurface.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxvertexcachedeformer.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/geometry/fbxweightedmapping.h"

//---------------------------------------------------------------------------------------
//Scene Shading Includes
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxshadingconventions.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxbindingsentryview.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxbindingtable.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxbindingtableentry.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxbindingoperator.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxconstantentryview.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxentryview.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxfiletexture.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbximplementation.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbximplementationfilter.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbximplementationutils.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxlayeredtexture.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxoperatorentryview.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxproceduraltexture.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxpropertyentryview.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxsemanticentryview.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxsurfacelambert.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxsurfacematerial.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxsurfacematerialutils.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxsurfacephong.h"
#include "FBX/fbxsdk/include/fbxsdk/scene/shading/fbxtexture.h"

//---------------------------------------------------------------------------------------
//Utilities Includes
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxdeformationsevaluator.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxprocessor.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxprocessorxref.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxprocessorxrefuserlib.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxprocessorshaderdependency.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxclonemanager.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxgeometryconverter.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxmanipulators.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxmaterialconverter.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxrenamingstrategyfbx5.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxrenamingstrategyfbx6.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxrenamingstrategyutilities.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxrootnodeutility.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxusernotification.h"
#include "FBX/fbxsdk/include/fbxsdk/utils/fbxscenecheckutility.h"

//---------------------------------------------------------------------------------------
#if defined(FBXSDK_NAMESPACE) && (FBXSDK_NAMESPACE_USING == 1)
	using namespace FBXSDK_NAMESPACE;
#endif

#endif /* _FBXSDK_H_ */
