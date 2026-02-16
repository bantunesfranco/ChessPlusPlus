#include <algorithm>
#include <stdexcept>

#include "../include/chess/types.hpp"
#include <string>
namespace chess
{
/*
    Gets the square index for the given square *name*
    (e.g., ``a1`` returns ``0``).
    throws `std::out_of_range` if the square name is invalid.
*/
Square parse_square(const std::string& name)
{
    const auto it = std::ranges::find(SQUARE_NAMES, name);
    if (it == SQUARE_NAMES.end())
        throw std::out_of_range("Square name not found");
    const auto idx = std::distance(SQUARE_NAMES.begin(), it);
    return SQUARES[idx];
}

/*
    Gets the name of the square, like ``a3``.
*/
const std::string& square_name(const Square square)
{
    return SQUARE_NAMES[square];
}

/*
    Gets a square number by file and rank index.
*/
Square square(const File file_index, const Rank rank_index)
{
    return (Square)(rank_index * 8 + file_index);
}

/*
    Gets the file index for the given file *name*
    (e.g., ``a`` returns ``0``).
    throws `std::out_of_range` if the file name is invalid.
*/
File parse_file(const std::string& name)
{
    const auto it = std::ranges::find(FILE_NAMES, name);
    if (it == FILE_NAMES.end())
        throw std::out_of_range("File name not found");
    const auto idx = std::ranges::distance(FILE_NAMES.begin(), it);
    return FILES[idx];
}

/*
    Gets the name of the square, like ``a``.
*/
const std::string& file_name(const File file)
{
    return FILE_NAMES[file];
}

/*
    Gets the rank index for the given rank *name*
    (e.g., ``1`` returns ``0``).
    throws `std::out_of_range` if the rank name is invalid.
*/
Rank parse_rank(const std::string& name)
{
    const auto it = std::ranges::find(RANK_NAMES, name);
    if (it == RANK_NAMES.end())
        throw std::out_of_range("Rank name not found");
    const auto idx = std::distance(RANK_NAMES.begin(), it);
    return RANKS[idx];
}

/*
    Gets the name of the rank, like ``1``.
*/
const std::string& rank_name(const Rank rank)
{
     return RANK_NAMES[rank];
}



}