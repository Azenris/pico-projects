
#pragma once

#include "types.h"

void random_set_seed( u64 seed );
void random_set_seed( const u64 seed[ 9 ] );
void random_get_state( u64 state[ 9 ] );

/// @func irandom( max )
/// @desc Return a random number ranged from 0 to max (inclusive)
/// @param	{u32}	max (inclusive)
/// @return	{u32}	random_number
[[nodiscard]] u32 irandom( u32 max );

/// @func irandom_range( min, max )
/// @desc Return a random number ranged from min to max (inclusive)
/// @param	{u32}	min (inclusive)
/// @param	{u32}	max (inclusive)
/// @return	{u32}	random_number
[[nodiscard]] u32 irandom_range( u32 min, u32 max );

/// @func irandom( max )
/// @desc Return a random number ranged from 0 to max (inclusive)
/// @param	{i32}	max (inclusive)
/// @return	{i32}	random_number
[[nodiscard]] i32 irandom( i32 max );

/// @func irandom_range( min, max )
/// @desc Return a random number ranged from min to max (inclusive)
/// @param	{i32}	min (inclusive)
/// @param	{i32}	max (inclusive)
/// @return	{i32}	random_number
[[nodiscard]] i32 irandom_range( i32 min, i32 max );

/// @func irandom64( max )
/// @desc Return a random number ranged from 0 to max (inclusive)
/// @param	{i64}	max (inclusive)
/// @return	{i64}	random_number
[[nodiscard]] i64 irandom( i64 max );

/// @func irandom_range64( min, max )
/// @desc Return a random number ranged from min to max (inclusive)
/// @param	{i64}	min (inclusive)
/// @param	{i64}	max (inclusive)
/// @return	{i64}	random_number
[[nodiscard]] i64 irandom_range( i64 min, i64 max );

/// @func irandom64( max )
/// @desc Return a random number ranged from 0 to max (inclusive)
/// @param	{u64}	max (inclusive)
/// @return	{u64}	random_number
[[nodiscard]] u64 irandom( u64 max );

/// @func irandom_range64( min, max )
/// @desc Return a random number ranged from min to max (inclusive)
/// @param	{u64}	min (inclusive)
/// @param	{u64}	max (inclusive)
/// @return	{u64}	random_number
[[nodiscard]] u64 irandom_range( u64 min, u64 max );

/// @func random( max )
/// @desc Return a random number ranged from 0 to max (inclusive)
/// @param	{f32}	max (inclusive)
/// @return	{f32}	random_number
[[nodiscard]] f32 random( f32 max );

/// @func random_range( min, max )
/// @desc Return a random number ranged from min to max (inclusive)
/// @param	{f32}	min (inclusive)
/// @param	{f32}	max (inclusive)
/// @return	{f32}	random_number
[[nodiscard]] f32 random_range( f32 min, f32 max );

/// @func random_f32_0_1()
/// @desc Return a random number ranged from [0, 1)
/// @return	{f32}	random_number
[[nodiscard]] f32 random_f32_0_1();

/// @func random_f64_0_1()
/// @desc Return a random number ranged from [0, 1)
/// @return	{f64}	random_number
[[nodiscard]] f64 random_f64_0_1();

/// @func proc( chance )
/// @desc Percent chance to proc something (inclusive)
/// @param	{i32}	chance (0-100)
/// @return	{bool}	proc
[[nodiscard]] bool iproc( i32 chance );

/// @func proc( chance )
/// @desc Percent chance to proc something (inclusive)
/// @param	{i64}	chance (0-100)
/// @return	{bool}	proc
[[nodiscard]] bool iproc( i64 chance );

/// @func proc( chance )
/// @desc Percent chance to proc something (inclusive)
/// @param	{u64}	chance (0-100)
/// @return	{bool}	proc
[[nodiscard]] bool iproc( u64 chance );

/// @func proc( chance )
/// @desc Percent chance to proc something (inclusive)
/// @param	{f32}	chance : upto 5 decimal places max (0-100)
/// @return	{bool}	proc
[[nodiscard]] bool proc( f32 chance );