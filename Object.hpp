#pragma once
/*
Provides a generic class to inherit from.
*/
namespace ee {
	class object {
		virtual bool isObject() { return true; }
	};
};