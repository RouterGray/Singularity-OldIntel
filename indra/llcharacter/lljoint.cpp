/** 
 * @file lljoint.cpp
 * @brief Implementation of LLJoint class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "lljoint.h"

#include "llmath.h"

S32 LLJoint::sNumUpdates = 0;
S32 LLJoint::sNumTouches = 0;

template <class T> 
bool attachment_map_iter_compare_key(const T& a, const T& b)
{
	return a.first < b.first;
}

bool LLPosOverrideMap::findActiveOverride(LLUUID& mesh_id, LLVector3& pos) const
{
	pos = LLVector3(0,0,0);
	mesh_id = LLUUID();
	bool found = false;
	
	map_type::const_iterator it = std::max_element(m_map.begin(),
												   m_map.end(),
												   attachment_map_iter_compare_key<map_type::value_type>);
	if (it != m_map.end())
	{
		found = true;
		pos = it->second;
		mesh_id = it->first;
	}
	return found;
}

void LLPosOverrideMap::showJointPosOverrides( std::ostringstream& os ) const
{
	map_type::const_iterator max_it = std::max_element(m_map.begin(),
													   m_map.end(),
													   attachment_map_iter_compare_key<map_type::value_type>);
	for (map_type::const_iterator it = m_map.begin();
		 it != m_map.end(); ++it)
	{
		const LLVector3& pos = it->second;
		os << " " << "[" << it->first <<": " << pos << "]" << ((it==max_it) ? "*" : "");
	}
}

U32 LLPosOverrideMap::count() const
{
	return m_map.size();
}

void LLPosOverrideMap::add(const LLUUID& mesh_id, const LLVector3& pos)
{
	m_map[mesh_id] = pos;
}

bool LLPosOverrideMap::remove(const LLUUID& mesh_id)
{
	U32 remove_count = m_map.erase(mesh_id);
	return (remove_count > 0);
}

void LLPosOverrideMap::clear()
{
	m_map.clear();
}
//-----------------------------------------------------------------------------
// LLJoint()
// Class Constructor
//-----------------------------------------------------------------------------


void LLJoint::init()
{
	mName = "unnamed";
	mParent = NULL;
	mXform.setScaleChildOffset(TRUE);
	mXform.setScale(LLVector3(1.0f, 1.0f, 1.0f));
	mDirtyFlags = MATRIX_DIRTY | ROTATION_DIRTY | POSITION_DIRTY;
	mUpdateXform = TRUE;
}

LLJoint::LLJoint() :
	mJointNum(-1)
{
	init();
	touch();
}

LLJoint::LLJoint(S32 joint_num) :
	mJointNum(joint_num)
{
	init();
	touch();
}


//-----------------------------------------------------------------------------
// LLJoint()
// Class Constructor
//-----------------------------------------------------------------------------
LLJoint::LLJoint(const std::string &name, LLJoint *parent) :
	mJointNum(0)
{
	init();
	mUpdateXform = FALSE;
	// *TODO: mResetAfterRestoreOldXform is not initialized!!!

	setName(name);
	if (parent)
	{
		parent->addChild( this );
	}
	touch();
}

//-----------------------------------------------------------------------------
// ~LLJoint()
// Class Destructor
//-----------------------------------------------------------------------------
LLJoint::~LLJoint()
{
	if (mParent)
	{
		mParent->removeChild( this );
	}
	removeAllChildren();
}


//-----------------------------------------------------------------------------
// setup()
//-----------------------------------------------------------------------------
void LLJoint::setup(const std::string &name, LLJoint *parent)
{
	setName(name);
	if (parent)
	{
		parent->addChild( this );
	}
}

//-----------------------------------------------------------------------------
// touch()
// Sets all dirty flags for all children, recursively.
//-----------------------------------------------------------------------------
void LLJoint::touch(U32 flags)
{
	if ((flags | mDirtyFlags) != mDirtyFlags)
	{
		sNumTouches++;
		mDirtyFlags |= flags;
		U32 child_flags = flags;
		if (flags & ROTATION_DIRTY)
		{
			child_flags |= POSITION_DIRTY;
		}

		for (child_list_t::iterator iter = mChildren.begin();
			 iter != mChildren.end(); ++iter)
		{
			LLJoint* joint = *iter;
			joint->touch(child_flags);
		}
	}
}

//-----------------------------------------------------------------------------
// getRoot()
//-----------------------------------------------------------------------------
LLJoint *LLJoint::getRoot()
{
	if ( getParent() == NULL )
	{
		return this;
	}
	return getParent()->getRoot();
}


//-----------------------------------------------------------------------------
// findJoint()
//-----------------------------------------------------------------------------
LLJoint *LLJoint::findJoint( const std::string &name )
{
	if (name == getName())
		return this;

	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLJoint* joint = *iter;
		LLJoint *found = joint->findJoint(name);
		if (found)
		{
			return found;
		}
	}

	return NULL;	
}


//--------------------------------------------------------------------
// addChild()
//--------------------------------------------------------------------
void LLJoint::addChild(LLJoint* joint)
{
	if (joint->mParent)
		joint->mParent->removeChild(joint);

	mChildren.push_back(joint);
	joint->mXform.setParent(&mXform);
	joint->mParent = this;	
	joint->touch();
}


//--------------------------------------------------------------------
// removeChild()
//--------------------------------------------------------------------
void LLJoint::removeChild(LLJoint* joint)
{
	child_list_t::iterator iter = std::find(mChildren.begin(), mChildren.end(), joint);
	if (iter != mChildren.end())
	{
		mChildren.erase(iter);
	
		joint->mXform.setParent(NULL);
		joint->mParent = NULL;
		joint->touch();
	}
}


//--------------------------------------------------------------------
// removeAllChildren()
//--------------------------------------------------------------------
void LLJoint::removeAllChildren()
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end();)
	{
		child_list_t::iterator curiter = iter++;
		LLJoint* joint = *curiter;
		mChildren.erase(curiter);
		joint->mXform.setParent(NULL);
		joint->mParent = NULL;
		joint->touch();
	}
}


//--------------------------------------------------------------------
// getPosition()
//--------------------------------------------------------------------
const LLVector3& LLJoint::getPosition()
{
	return mXform.getPosition();
}

bool do_debug_joint(const std::string& name)
{
	return false;
}

//--------------------------------------------------------------------
// setPosition()
//--------------------------------------------------------------------
void LLJoint::setPosition( const LLVector3& pos )
{
	if (pos != getPosition())
	{
		if (do_debug_joint(getName()))
		{
			LL_DEBUGS("Avatar") << " joint " << getName() << " set pos " << pos << LL_ENDL;
		}
	}
	mXform.setPosition(pos);
	touch(MATRIX_DIRTY | POSITION_DIRTY);
}

void showJointPosOverrides( const LLJoint& joint, const std::string& note, const std::string& av_info )
{
        std::ostringstream os;
        os << joint.m_posBeforeOverrides;
        joint.m_attachmentOverrides.showJointPosOverrides(os);
        LL_DEBUGS("Avatar") << av_info << " joint " << joint.getName() << " " << note << " " << os.str() << LL_ENDL;
}

//--------------------------------------------------------------------
// addAttachmentPosOverride()
//--------------------------------------------------------------------
void LLJoint::addAttachmentPosOverride( const LLVector3& pos, const LLUUID& mesh_id, const std::string& av_info )
{
	if (mesh_id.isNull())
	{
		return;
	}
	if (!m_attachmentOverrides.count())
	{
		if (do_debug_joint(getName()))
		{
			LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " saving m_posBeforeOverrides " << getPosition() << LL_ENDL;
		}
		m_posBeforeOverrides = getPosition();
	}
	m_attachmentOverrides.add(mesh_id,pos);
	if (do_debug_joint(getName()))
	{
		LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " addAttachmentPosOverride for mesh " << mesh_id << " pos " << pos << LL_ENDL;
	}
	updatePos(av_info);
}

//--------------------------------------------------------------------
// removeAttachmentPosOverride()
//--------------------------------------------------------------------
void LLJoint::removeAttachmentPosOverride( const LLUUID& mesh_id, const std::string& av_info )
{
	if (mesh_id.isNull())
	{
		return;
	}
	if (m_attachmentOverrides.remove(mesh_id))
	{
		if (do_debug_joint(getName()))
		{
			LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName()
								<< " removeAttachmentPosOverride for " << mesh_id << LL_ENDL;
			showJointPosOverrides(*this, "remove", av_info);
		}
		updatePos(av_info);
	}
}

//--------------------------------------------------------------------
 // hasAttachmentPosOverride()
 //--------------------------------------------------------------------
bool LLJoint::hasAttachmentPosOverride( LLVector3& pos, LLUUID& mesh_id ) const
{
	return m_attachmentOverrides.findActiveOverride(mesh_id,pos);
}

//--------------------------------------------------------------------
// clearAttachmentPosOverrides()
//--------------------------------------------------------------------
void LLJoint::clearAttachmentPosOverrides()
{
	if (m_attachmentOverrides.count())
	{
		m_attachmentOverrides.clear();
		setPosition(m_posBeforeOverrides);
		setId( LLUUID::null );
	}
}

//--------------------------------------------------------------------
// updatePos()
//--------------------------------------------------------------------
void LLJoint::updatePos(const std::string& av_info)
{
	LLVector3 pos, found_pos;
	LLUUID mesh_id;
	if (m_attachmentOverrides.findActiveOverride(mesh_id,found_pos))
	{
		LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " updatePos, winner of " << m_attachmentOverrides.count() << " is mesh " << mesh_id << " pos " << found_pos << LL_ENDL;
		pos = found_pos;
	}
	else
	{
		LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " updatePos, winner is posBeforeOverrides " << m_posBeforeOverrides << LL_ENDL;
		pos = m_posBeforeOverrides;
	}
	setPosition(pos);
}

//--------------------------------------------------------------------
// getWorldPosition()
//--------------------------------------------------------------------
LLVector3 LLJoint::getWorldPosition()
{
	updateWorldPRSParent();
	return mXform.getWorldPosition();
}

//-----------------------------------------------------------------------------
// getLastWorldPosition()
//-----------------------------------------------------------------------------
LLVector3 LLJoint::getLastWorldPosition()
{
	return mXform.getWorldPosition();
}


//--------------------------------------------------------------------
// setWorldPosition()
//--------------------------------------------------------------------
void LLJoint::setWorldPosition( const LLVector3& pos )
{
	if (mParent == NULL)
	{
		this->setPosition( pos );
		return;
	}

	LLMatrix4a temp_matrix = getWorldMatrix();
	temp_matrix.setTranslate_affine(pos);

	LLMatrix4a invParentWorldMatrix = mParent->getWorldMatrix();
	invParentWorldMatrix.invert();

	invParentWorldMatrix.mul(temp_matrix);

	LLVector3 localPos(	invParentWorldMatrix.getRow<LLMatrix4a::ROW_TRANS>().getF32ptr() );

	setPosition( localPos );
}


//--------------------------------------------------------------------
// mXform.getRotation()
//--------------------------------------------------------------------
const LLQuaternion& LLJoint::getRotation()
{
	return mXform.getRotation();
}


//--------------------------------------------------------------------
// setRotation()
//--------------------------------------------------------------------
void LLJoint::setRotation( const LLQuaternion& rot )
{
	if (rot.isFinite())
	{
	//	if (mXform.getRotation() != rot)
		{
			mXform.setRotation(rot);
			touch(MATRIX_DIRTY | ROTATION_DIRTY);
		}
	}
}


//--------------------------------------------------------------------
// getWorldRotation()
//--------------------------------------------------------------------
LLQuaternion LLJoint::getWorldRotation()
{
	updateWorldPRSParent();

	return mXform.getWorldRotation();
}

//-----------------------------------------------------------------------------
// getLastWorldRotation()
//-----------------------------------------------------------------------------
LLQuaternion LLJoint::getLastWorldRotation()
{
	return mXform.getWorldRotation();
}

//--------------------------------------------------------------------
// setWorldRotation()
//--------------------------------------------------------------------
void LLJoint::setWorldRotation( const LLQuaternion& rot )
{
	if (mParent == NULL)
	{
		this->setRotation( rot );
		return;
	}
	
	LLMatrix4a parentWorldMatrix = mParent->getWorldMatrix();
	LLQuaternion2 rota(rot);
	LLMatrix4a temp_mat(rota);

	LLMatrix4a invParentWorldMatrix = mParent->getWorldMatrix();
	invParentWorldMatrix.setTranslate_affine(LLVector3(0.f));

	invParentWorldMatrix.invert();

	invParentWorldMatrix.mul(temp_mat);

	setRotation(LLQuaternion(LLMatrix4(invParentWorldMatrix.getF32ptr())));
}


//--------------------------------------------------------------------
// getScale()
//--------------------------------------------------------------------
const LLVector3& LLJoint::getScale()
{
	return mXform.getScale();
}

//--------------------------------------------------------------------
// setScale()
//--------------------------------------------------------------------
void LLJoint::setScale( const LLVector3& scale )
{
//	if (mXform.getScale() != scale)
	{
		mXform.setScale(scale);
		touch();
	}

}



//--------------------------------------------------------------------
// getWorldMatrix()
//--------------------------------------------------------------------
const LLMatrix4a &LLJoint::getWorldMatrix()
{
	updateWorldMatrixParent();

	return mXform.getWorldMatrix();
}


//--------------------------------------------------------------------
// setWorldMatrix()
//--------------------------------------------------------------------
void LLJoint::setWorldMatrix( const LLMatrix4& mat )
{
LL_INFOS() << "WARNING: LLJoint::setWorldMatrix() not correctly implemented yet" << LL_ENDL;
	// extract global translation
	LLVector3 trans(	mat.mMatrix[VW][VX],
						mat.mMatrix[VW][VY],
						mat.mMatrix[VW][VZ] );

	// extract global rotation
	LLQuaternion rot( mat );

	setWorldPosition( trans );
	setWorldRotation( rot );
}

//-----------------------------------------------------------------------------
// updateWorldMatrixParent()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldMatrixParent()
{
	if (mDirtyFlags & MATRIX_DIRTY)
	{
		LLJoint *parent = getParent();
		if (parent)
		{
			parent->updateWorldMatrixParent();
		}
		updateWorldMatrix();
	}
}

//-----------------------------------------------------------------------------
// updateWorldPRSParent()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldPRSParent()
{
	if (mDirtyFlags & (ROTATION_DIRTY | POSITION_DIRTY))
	{
		LLJoint *parent = getParent();
		if (parent)
		{
			parent->updateWorldPRSParent();
		}

		mXform.update();
		mDirtyFlags &= ~(ROTATION_DIRTY | POSITION_DIRTY);
	}
}

//-----------------------------------------------------------------------------
// updateWorldMatrixChildren()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldMatrixChildren()
{	
	if (!this->mUpdateXform) return;

	if (mDirtyFlags & MATRIX_DIRTY)
	{
		updateWorldMatrix();
	}
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLJoint* joint = *iter;
		joint->updateWorldMatrixChildren();
	}
}

//-----------------------------------------------------------------------------
// updateWorldMatrix()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldMatrix()
{
	if (mDirtyFlags & MATRIX_DIRTY)
	{
		sNumUpdates++;
		mXform.updateMatrix(FALSE);
		mDirtyFlags = 0x0;
	}
}

//--------------------------------------------------------------------
// getSkinOffset()
//--------------------------------------------------------------------
const LLVector3 &LLJoint::getSkinOffset()
{
	return mSkinOffset;
}


//--------------------------------------------------------------------
// setSkinOffset()
//--------------------------------------------------------------------
void LLJoint::setSkinOffset( const LLVector3& offset )
{
	mSkinOffset = offset;
}


//-----------------------------------------------------------------------------
// clampRotation()
//-----------------------------------------------------------------------------
void LLJoint::clampRotation(LLQuaternion old_rot, LLQuaternion new_rot)
{
	LLVector3 main_axis(1.f, 0.f, 0.f);

	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLJoint* joint = *iter;
		if (joint->isAnimatable())
		{
			main_axis = joint->getPosition();
			main_axis.normVec();
			// only care about first animatable child
			break;
		}
	}

	// 2003.03.26 - This code was just using up cpu cycles. AB

//	LLVector3 old_axis = main_axis * old_rot;
//	LLVector3 new_axis = main_axis * new_rot;

//	for (S32 i = 0; i < mConstraintSilhouette.count() - 1; i++)
//	{
//		LLVector3 vert1 = mConstraintSilhouette[i];
//		LLVector3 vert2 = mConstraintSilhouette[i + 1];

		// figure out how to clamp rotation to line on 3-sphere

//	}
}

// <edit>
std::string LLJoint::exportString(U32 tabs)
{
	std::string out;
	for(U32 i = 0; i < tabs; i++) out.append("\t");
	out.append("<joint ");
	out.append(llformat("name=\"%s\" ", this->mName.c_str()));
	LLVector3 position = getPosition();
	out.append(llformat("pos=\"<%f, %f, %f>\" ", position.mV[0], position.mV[1], position.mV[2]));
	LLQuaternion rotation = getRotation();
	out.append(llformat("rot=\"<%f, %f, %f, %f>\" ", rotation.mQ[0], rotation.mQ[1], rotation.mQ[2], rotation.mQ[3]));
	if(mChildren.empty())
	{
		out.append("/>\n");
	}
	else
	{
		out.append(">\n");
		int next_tab = tabs + 1;
		child_list_t::iterator end = mChildren.end();
		for(child_list_t::iterator iter = mChildren.begin(); iter != end; ++iter)
		{
			out.append((*iter)->exportString(next_tab));
		}
		for(U32 i = 0; i < tabs; i++) out.append("\t");
		out.append("</joint>\n");
	}
	return out;
}
// </edit>

// End
