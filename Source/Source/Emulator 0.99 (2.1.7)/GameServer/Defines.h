#pragma once

#define DECLARE_ENUM(type,name)\
	protected:\
		##type m_prop##name; \
	public: \
		##type Get##name() const {return this->m_prop##name;}\
		void Set##name(##type value) {this->m_prop##name = value;}