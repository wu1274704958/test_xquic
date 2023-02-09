#pragma once

/// <summary>
/// contain(EngineFlags::HTTP_Server,EngineFlags::HTTP)
/// </summary>
/// <typeparam name="E">enum type</typeparam>
/// <typeparam name="NT">Actual value type</typeparam>
/// <param name="f1">enum val 1</param>
/// <param name="f2">enum val 2</param>
/// <returns></returns>
template<typename NT,typename E>
bool contain(E f1, E f2)
{
	return (static_cast<NT>(f1) & static_cast<NT>(f2)) == static_cast<NT>(f2);
}