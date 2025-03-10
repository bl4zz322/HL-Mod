/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum deagle_e {
	DEAGLE_IDLE1 = 0,
	DEAGLE_IDLE2,
	DEAGLE_IDLE3,
	DEAGLE_IDLE4,
	DEAGLE_IDLE5,
	DEAGLE_SHOOT,
	DEAGLE_SHOOT_EMPTY,
	DEAGLE_RELOAD,
	DEAGLE_RELOAD_NOT_EMPTY,
	DEAGLE_DRAW,
	DEAGLE_HOLSTER
};

class CDesertEagle : public CBasePlayerWeapon
{
public:
	void Spawn(override);
	void Precache(override);
	int iItemSlot(override) { return 2; }
	int GetItemInfo(ItemInfo* p);

	void PrimaryAttack(override);
	void SecondaryAttack(override);
	void DeagleFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);
	BOOL Deploy(override);
	void Reload(override);
	void WeaponIdle(override);

private:
	int m_iShell;

	unsigned short m_usFireDeagle;
};
LINK_ENTITY_TO_CLASS(weapon_deagle, CDesertEagle);

void CDesertEagle::Spawn()
{
	Precache();
	m_iId = WEAPON_DEAGLE;
	SET_MODEL(ENT(pev), "models/w_desert_eagle.mdl");

	m_iDefaultAmmo = DEAGLE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CDesertEagle::Precache(void)
{
	PRECACHE_MODEL("models/v_desert_eagle.mdl");
	PRECACHE_MODEL("models/w_desert_eagle.mdl");
	PRECACHE_MODEL("models/p_desert_eagle.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/pl_gun1.wav");//silenced handgun
	PRECACHE_SOUND("weapons/pl_gun2.wav");//silenced handgun
	PRECACHE_SOUND("weapons/pl_gun3.wav");//handgun

	m_usFireDeagle = PRECACHE_EVENT(1, "events/deagle.sc");
}

int CDesertEagle::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DEAGLE_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_DEAGLE;
	p->iWeight = DEAGLE_WEIGHT;

	return 1;
}

BOOL CDesertEagle::Deploy()
{
	// pev->body = 1;
	return DefaultDeploy("models/v_desert_eagle.mdl", "models/p_desert_eagle.mdl", DEAGLE_DRAW, "onehanded");
}

void CDesertEagle::SecondaryAttack(void)
{
	DeagleFire(0.1, 0.2, FALSE);
}

void CDesertEagle::PrimaryAttack(void)
{
	DeagleFire(0.01, 0.3, TRUE);
}

void CDesertEagle::GlockFire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = gpGlobals->time + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

#if defined ( OLD_WEAPONS )
	if (m_iClip != 0)
		SendWeaponAnim(DEAGLE_SHOOT);
	else
		SendWeaponAnim(DEAGLE_SHOOT_EMPTY);
#endif

	if (fUseAutoAim)
	{
		PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usFireDeagle, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, 0.0, 0.0, 0, 0, (m_iClip == 0) ? 1 : 0, 0);
	
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#if defined ( OLD_WEAPONS )
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
		+ gpGlobals->v_right * RANDOM_FLOAT(50, 70)
		+ gpGlobals->v_up * RANDOM_FLOAT(100, 150)
		+ gpGlobals->v_forward * 25;
	EjectBrass(pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 + gpGlobals->v_right * 6, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
#endif

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
#if defined ( OLD_WEAPONS )
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun1.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun2.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
			break;
		}
#endif
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
#if defined ( OLD_WEAPONS )
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun3.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
#endif
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	if (fUseAutoAim)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT(10, 15);

#if defined ( OLD_WEAPONS )
	m_pPlayer->pev->punchangle.x -= 2;
#endif
}


void CDesertEagle::Reload(void)
{
	int iResult = DefaultReload(DEAGLE_MAX_CLIP, m_iClip > 0 ? DEAGLE_RELOAD_NOT_EMPTY : DEAGLE_RELOAD, 1.5);
	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT(10, 15);
	}
}



void CDesertEagle::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer > random_seed, 0.0, 1.0);

		if (flRand  <= 0.3
		{
			iAnim = DEAGLE_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 76.0f / 30.0f;
		}
		else if (flRand <= 0.6)
		{
			iAnim = DEAGLE_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61.0f / 24.0f;
		}
		else
		{
			iAnim = DEAGLE_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 50.0f / 30.0f;
		}
		SendWeaponAnim(iAnim);
	}
}



