#include <stdint.h>   //uint#_t types
//#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <iostream>
#include <string>

int balanced_random(int maxRandomRange)
{
    //higher rand() than maxValid (on mod/%) wraps around to 0 but doesn't reach maxRandomRange (on mod/%) at RAND_MAX,
    //so above maxValid is biased toward lower output. Simple solution: discard values above maxValid and roll again
    int maxValid = RAND_MAX - (RAND_MAX+1)%(maxRandomRange+1);
    int rawRand = rand();
    while (rawRand > maxValid)
        rawRand = rand();  //rand() returns int

    return rawRand%(maxRandomRange+1);
}

class MinesweeperTile
{
public:
    bool isRevealed;
    bool isMine;
    bool isFlagged;
    uint8_t adjacentMines;

    MinesweeperTile(bool _isMine=false) : isMine(_isMine), isRevealed(false), isFlagged(false), adjacentMines(0) {}
    void toggle_flag(void) { isFlagged = !isFlagged; }
    bool reveal_tile(void) { isRevealed = true; return isMine; }  // returns true if a mine (and we lose!)
    friend std::ostream& operator<<(std::ostream& stream, const MinesweeperTile& tile);  //not a member function of MinsweeperTile (below)
};

std::ostream& operator<<(std::ostream& stream, const MinesweeperTile& tile)
{
    if (!tile.isRevealed)
    {
        if (!tile.isFlagged)        stream << '.';
        else                        stream << '<';
    }
    else if (tile.isMine)           stream << 'X';
    else if (tile.adjacentMines==0) stream << ' ';
    else                            stream << (char)(tile.adjacentMines+48);
    return stream;
}



class MinesweeperBoard
{
public:
    MinesweeperTile* boardTiles;
    size_t bWidth, bHeight;
    uint32_t mineTotalCount;

    //constructor
    MinesweeperBoard(size_t _boardWidth, size_t _boardHeight, uint32_t _mineCount) : bWidth(_boardWidth), bHeight(_boardHeight), mineTotalCount(_mineCount)
    {
        assert(bWidth*bHeight > mineTotalCount);   //can't have more mines than tiles. probably need like a 2:1 ratio minimum but we don't enforce that
        //boardTiles = (MinesweeperTile*) malloc( sizeof(MinesweeperTile) * bWidth*bHeight);  //c-style
        //std::vector<MinesweeperTile> boardTile(bWidth*bHeight);  //reasonable-style
        boardTiles = new MinesweeperTile[bWidth*bHeight];
        for (size_t tileNo=0; tileNo<bWidth*bHeight; ++tileNo)
            boardTiles[tileNo] = MinesweeperTile();
    }

    ~MinesweeperBoard(void)
    {
        delete[] boardTiles;
        //free(boardTiles);  //c-style
    }

    //randomly place the mines and calculate the numbers we display
    void lay_mines(size_t safeIdx)
    {
          boardTiles[safeIdx].isMine = true;  //this tile should be the one the user clicked. it's guaranteed safe (we'll remove mine after placing mines, mined now so we don;t place later)

        //randomly choose tiles to be mines
        size_t tileCount = bWidth*bHeight;
        size_t mineNo = 0;
        while (mineNo < mineTotalCount)
        {
            size_t idx = balanced_random(tileCount);
            if (!boardTiles[idx].isMine)
            {
                boardTiles[idx].isMine = true;  //###make setter/getter for MinesweeperTile.isMine and set private
                ++mineNo;
                     //int y = idx/bWidth, x = idx%bWidth;
                     //printf( "placed mine #%d at (%d,%d)\n", mineNo, x, y);
                     //std::cout << "placed mine " << mineNo << " at "
            }
        }

          boardTiles[safeIdx].isMine = false;

        //calculate the adjacentMine values for each tile
        //outer 2 loops loop over tiles. inner 2 loops loop over the 8-connected neighbors of the tile to count the mines
        for (size_t rowNo=0; rowNo<bHeight; ++rowNo)
            for (size_t colNo=0; colNo<bWidth; ++colNo)
            {
                MinesweeperTile* pTile = &boardTiles[rowNo*bWidth + colNo];

                //we don't count mines for mine tiles (reveal mine = lose)
                if ( pTile->isMine )
                    continue;

                //count adjacent mines. note we need to consider bounds around the board edges (row/col 0/end)
                int adjacentMineCount = 0;
                for (size_t subRow=(rowNo==0?0:rowNo-1); subRow<=(rowNo==bHeight-1?bHeight-1:rowNo+1); ++subRow)
                    for (size_t subCol=(colNo==0?0:colNo-1); subCol<=(colNo==bWidth-1?bWidth-1:colNo+1); ++subCol)
                    {
                        //we don't need to skip the center tile (pTile) - we know it's not a mine so it won't increase the count anyway
                        if (boardTiles[subRow*bWidth + subCol].isMine)
                            ++adjacentMineCount;
                    }

                pTile->adjacentMines = adjacentMineCount;
            }


    }

    //get the minesweeperTile at (row,col)
    MinesweeperTile& operator() (size_t row, size_t col) const
    {
        assert (row<=bHeight && col<=bWidth);
        return boardTiles[row*bWidth + col];
    }

    friend std::ostream& operator<<(std::ostream& stream, const MinesweeperBoard& board);  //not a member function
};

std::ostream& operator<<(std::ostream& stream, const MinesweeperBoard& board)
{
     for (size_t colNo=0; colNo<board.bWidth; ++colNo)
     {
         std::string colName = std::to_string(colNo);
         if (colName.size() < 2)  colName = " " + colName + " ";
         else if (colName.size() < 3) colName = " " + colName;
         stream << colName;
     }
     stream << std::endl;
     for (size_t colNo=0; colNo<board.bWidth; ++colNo)		 stream << "___";
     stream << std::endl;

    for (size_t rowNo=0; rowNo<board.bHeight; ++rowNo)
    {
        for (size_t colNo=0; colNo<board.bWidth; ++colNo)
        {
            stream << " " << board(rowNo, colNo) << " ";
        }
        stream << "|" << rowNo << '\n';
    }
    return stream;
}



class MinesweeperGame
{
public:
    MinesweeperBoard gameBoard;
    int32_t mineRemainCount, tileRemainCount, solveTime; //mine count can be negative if you have erroneous mines. tile count can't be negative
    clock_t startTime;
    bool gameOver, gameStarted;

    MinesweeperGame(size_t _width=10, size_t _height=-1, uint32_t _mines=-1) :
        gameBoard( MinesweeperBoard(_width, (_height<0?1.618*_width:_height), (_mines<0?_width*_height*3/20:_mines) ) ),
        startTime(0),
        tileRemainCount(_width*_height - _mines),
        mineRemainCount(_mines),
        gameOver(false),
        gameStarted(false)
    {
        std::cout << gameBoard << std::endl;
    }

     void start_game(size_t safeIdx)
     {
         gameStarted = true;
         gameBoard.lay_mines(safeIdx);
         startTime = clock();
     }

     void flag_tile(size_t rowNo, size_t colNo)
     {
         MinesweeperTile* pTile = &gameBoard(rowNo,colNo);
         if (pTile->isFlagged)
             ++mineRemainCount;
         else
             --mineRemainCount;

         pTile->toggle_flag();

         //update board? send signal to UI that board state has changed, so re-render
         show_board();
     }

    //revealing tiles in minesweeper: 1)reveal a mine - you lose  2)reveal a non-mine - show the value  3)if value is 0, reveal all 8-connected adjacent tiles recursively
    void reveal_tile(size_t rowNo, size_t colNo)
    {
        //start timer on first reveal
        if (!gameStarted)
            start_game(rowNo*gameBoard.bWidth + colNo);

        reveal_tile_r(rowNo, colNo);

        //update board? send signal to UI that board state has changed, so re-render
        show_board();

        if (tileRemainCount<=0)
            game_win();
    }

    //###make private
    void reveal_tile_r(size_t rowNo, size_t colNo)
    {
        MinesweeperTile* pTile = &gameBoard(rowNo,colNo);

        //if tile already revealed, do nothing
        if (pTile->isRevealed)
            return;

        //if flagged, don't reveal even if clicked. that's what the flag is for
        if (pTile->isFlagged)
            return;

        //this can't happen as a result of cascading reveals, so we don't need to worry about other calls in stack executing after this game_lose/return
        if ( pTile->reveal_tile() )
        {
            game_lose();
            return;
        }

        --tileRemainCount;
        if (pTile->adjacentMines == 0)
        {
            for (size_t subRow=(rowNo==0?0:rowNo-1); subRow<=(rowNo==gameBoard.bHeight-1?gameBoard.bHeight-1:rowNo+1); ++subRow)
                for (size_t subCol=(colNo==0?0:colNo-1); subCol<=(colNo==gameBoard.bWidth-1?gameBoard.bWidth-1:colNo+1); ++subCol)
                {
                    //we don't need to skip the center tile (pTile). it's already revealed it stops there
                    this->reveal_tile_r(subRow,subCol);
                }
        }
    }

    uint32_t get_elapsed_time(void)
    {
        return (clock() - startTime) / CLOCKS_PER_SEC;
    }

    void game_lose(void)
    {
        std::cout << "loss!" << std::endl;
        gameOver = true;
    }

    void game_win(void)
    {
          solveTime = get_elapsed_time();
        std::cout << "win!" << "   time: " << solveTime << std::endl;
        gameOver = true;
    }

    void show_board(void)
    {
          std::cout << "time: " << get_elapsed_time() << "   mines left: " << mineRemainCount <<  "    tiles left: " << tileRemainCount << std::endl;
        std::cout << gameBoard << std::endl;
    }

    //look for "##,##(f)"  -  2 numbers with a comma between and no space. then possibly an 'f' somewhere to indicate "flag" (instead of click/reveal)
    bool parse_player_input(const std::string& inpStr)
    {
        if ( inpStr.find('q') != std::string::npos )  // if input contains 'q' anywhere, requested action is quit
            return false;

        int row=-1,col=-1;


        size_t commaPos = inpStr.find(','), pos=commaPos, inpStrLng=inpStr.size();
        if (commaPos == std::string::npos)
            return true;

        //get first number (row): start from the comma. step back until no longer a digit, then give it the atoi
        while (pos>0)
            if ( !isdigit(inpStr[--pos]) )  { ++pos; break; }

        row = atoi( inpStr.substr(pos, commaPos).c_str() );

        //get second number (col): start from the comma. step forward until no longer a digit, then give it the atoi
        pos = commaPos;
        while (pos<(inpStrLng-1))
            if ( !isdigit(inpStr[++pos]) )  { --pos; break; }

        col = atoi( inpStr.substr(commaPos+1, pos).c_str() );

        //if invalid row/col, don't perform any action. maybe we should say something... but for now we just ignore invalid input. return true: we don't want to quit
        if (row < 0 || row >= gameBoard.bHeight || col < 0 || col >= gameBoard.bWidth)
            return true;

        //check for flag 'f' (anywhere in string)... "f row,col", "frow,col", "row,colf", etc
        bool isFlag = ( inpStr.find('f') != std::string::npos );  // if input contains 'f' anywhere, we call it a flag request

        //perform requested action
        if (isFlag) flag_tile(row, col);
        else        reveal_tile(row,col);

        return true;
    }
};





int main(int argc, char *argv[])
{
    int w, h, m;
    if (argc>1) w = atoi(argv[1]);
    else w = 10;

    if (argc>2) h = atoi(argv[2]);
    else h = 1.618*w;

    if (argc>3) m = atoi(argv[3]);
    else m = 3*w*h/20;

    MinesweeperGame mswGame(w, h, m);
    std::string inpStr;

    while( !mswGame.gameOver )
    {
        std::cout << "enter \"row,col\": ";
        std::cin >> inpStr;
        if (!mswGame.parse_player_input(inpStr) )
            break;
    }

     getchar();
     getchar();

     return 0;
}
