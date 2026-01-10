// Copyright 2026 Ultimate Player All Rights Reserved.

#pragma once

#include "XiangQiPro/Util/ChessInfo.h"
#include "XiangQiPro/Util/ChessMove.h"
#include "XiangQiPro/Util/Clock.h"

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "AI2P.generated.h"

class UChessBoard2P;
class UTacticsLibrary2P;
class AChesses;

typedef UAI2P AI2P;
typedef UKismetMathLibrary Math;

// 置换表条目类型
UENUM()
enum class ETranspositionFlag : uint8
{
    Exact,   // 精确值
    Alpha,   // 下界（α值）
    Beta     // 上界（β值）
};

// 置换表条目
struct FTranspositionEntry
{
    uint64 ZobristKey;       // 局面哈希值
    int32 Depth;             // 搜索深度
    int32 Value;             // 评估值
    ETranspositionFlag Flag; // 条目类型
    FChessMove2P BestMove;   // 该局面的最优走法

    FTranspositionEntry() : ZobristKey(0), Depth(-1), Value(0), Flag(ETranspositionFlag::Exact) 
    {
        BestMove.bIsValid = false;
    }
};

// 走法评分（用于排序）
struct FMoveScore
{
    FChessMove2P Move;       // 走法
    int32 Score;             // 评分（越高优先级越高）

    FMoveScore(FChessMove2P InMove, int32 InScore) : Move(InMove), Score(InScore) {}
};

// AI2P.h 新增枚举
UENUM(BlueprintType)
enum class EAI2PDifficulty : uint8
{
    Easy = 0,   // 简单（高随机）
    Normal = 1, // 普通（中随机）
    Hard = 2    // 困难（低随机）
};

// 游戏阶段
UENUM()
enum class EChessGamePhase : uint8
{
    Opening,  // 开局
    Midgame,  // 中局
    Endgame   // 残局
};

// 关键点位类型（用于防守/进攻评估）
UENUM()
enum class EKeyChessPoint : uint8
{
    Center,      // 中路（Y=4）
    KingGate,    // 将门（红：X=0,Y=4；黑：X=9,Y=4）
    SoldierLine, // 兵线（红：X=4；黑：X=5）
    HorsePoint   // 马位（如3·2、7·2等经典马位）
};

// === 战术类型枚举 ===
UENUM()
enum class EChessTactic : uint8
{
    None,               // 无战术
    WoCaoMa,            // 卧槽马
    ChenDiPao,          // 沉底炮
    ZhongLuTuPo,        // 中路突破
    ShuangCheCuo,       // 双车错
    BingXianTuiJin      // 兵线推进
};

// === 战术评估结果 ===
struct FTacticEvalResult
{
    EChessTactic TacticType;  // 战术类型
    int32 FeasibilityScore;   // 可行性分值（0-100，越高越易成功）
    FChessMove2P BestTacticMove; // 执行该战术的最优走法

    FTacticEvalResult() : TacticType(EChessTactic::None), FeasibilityScore(0) {}
};

class UChessMLModule;

UCLASS()
class XIANGQIPRO_API UAI2P : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI2P")
    EAI2PDifficulty AIDifficulty = EAI2PDifficulty::Hard;

    // 构造函数
    UAI2P();

    // 核心：获取AI最优走法
    FChessMove2P GetBestMove(TWeakObjectPtr<UChessBoard2P> InBoard2P, EChessColor InAiColor, EAI2PDifficulty InDifficulty, int32 InMaxTime = 10000, bool bEnableMachineLearning = false, UChessMLModule* MLModule = nullptr);

    // 获取玩家得分
    int32 Evaluate(EChessColor color);

    // 立刻停止搜索
    UFUNCTION(BlueprintCallable, Category = "Chess AI")
    void StopThinkingImmediately();

    // 设置棋盘引用
    void SetBoard(TWeakObjectPtr<UChessBoard2P> newBoard);

    // 检查走法是否阻挡了对方的攻击（优化后）
    bool IsBlockingAttack(const FChessMove2P& move, EChessColor color);

private:

    int32 MaxTime = 10000;

    FClock Clock;

    EChessGamePhase GamePhase = EChessGamePhase::Opening;

    EChessColor GlobalAIColor = EChessColor::BLACKCHESS;

    FChessMove2P OnlyOneMove = FChessMove2P();

    // 弱引用棋盘对象
    TWeakObjectPtr<UChessBoard2P> board2P;

    // 置换表（缓存棋盘局面）
    TMap<uint64, FTranspositionEntry> TranspositionTable;

    // 检查走法是否会导致棋子被无谓吃掉
    bool IsMoveSuicidal(const FChessMove2P& Move, EChessColor AiColor);

    // 过滤车/炮的无效移动（自将、攻击有根且无收益、超出棋盘等）
    TArray<FChessMove2P> FilterInvalidMoves(const TArray<FChessMove2P>& RawMoves, EChessColor AiColor, EChessType PieceType);

    // === 战术核心函数声明 ===
    // 评估当前局面最优战术（返回可行性最高的战术）
    FTacticEvalResult EvaluateBestTactic(EChessColor AiColor);
    // 识别卧槽马战术
    FTacticEvalResult RecognizeWoCaoMa(EChessColor AiColor);
    // 识别沉底炮战术
    FTacticEvalResult RecognizeChenDiPao(EChessColor AiColor);
    // 识别中路突破战术
    FTacticEvalResult RecognizeZhongLuTuPo(EChessColor AiColor);
    // 识别双车错战术
    FTacticEvalResult RecognizeShuangCheCuo(EChessColor AiColor);
    // 识别兵线推进战术
    FTacticEvalResult RecognizeBingXianTuiJin(EChessColor AiColor);

    // === 防守核心函数 ===
    // 预判对方是否有致命进攻（直接威胁将/帅）
    bool IsOpponentHasLethalAttack(EChessColor AiColor);
    // 评估防守弱点（返回弱点分值，值越低弱点越大）
    int32 EvaluateDefenseWeakness(EChessColor AiColor);
    // 检查是否存在双炮威胁
    bool HasDoublePaoThreat(EChessColor AiColor);
    // 专门评估双炮威胁
    int32 EvaluateDoublePaoThreat(EChessColor AiColor);
    // 检查是否立即的双炮杀
    bool IsImmediateDoublePaoMate(EChessColor AiColor);
    // 是否还能逃脱双炮
    bool CanKingEscapeDoublePao(EChessColor AiColor, const FChessMove2P& opponentMove);
    // 检查走法是否能防御双炮
    bool CanDefendAgainstDoublePao(const FChessMove2P& Move, EChessColor AiColor);
    // 检查是否阻挡炮线
    bool BlocksPaoLine(const FChessMove2P& Move, EChessColor AiColor);
    // 判断走法是否为有效防守（拦截致命进攻/补位弱点）
    bool IsEffectiveDefenseMove(const FChessMove2P& Move, EChessColor AiColor);

    // === 进攻核心函数 ===
    // 评估走法的进攻协同性（多棋子配合度）
    int32 EvaluateAttackSynergy(const FChessMove2P& Move, EChessColor AiColor);
    // 判断是否控制关键点位
    bool IsControlKeyPoint(int32 X, int32 Y, EKeyChessPoint PointType, EChessColor Color);

    // === 评估核心函数 ===

    // 开局专项评估
    int32 EvaluateOpeningFeatures(EChessColor AiColor);
    // 中后期专项评估
    int32 EvaluateMidEndgameFeatures(EChessColor AiColor);
    // 快速棋子出动评估
    int32 EvaluatePieceDevelopmentQuick(EChessColor Color);
    // 评估中路保护（开局阶段特别重要）
    int32 EvaluateCenterProtection(EChessColor AiColor);
    // 评估中路控制权
    int32 EvaluateCenterControl(EChessColor AiColor);
    // 评估兵种组合配合
    int32 EvaluatePieceCombination(int32 X, int32 Y, EChessColor Color);
    // 评估线路控制协同
    int32 EvaluateLineControlCooperation(int32 X, int32 Y, EChessColor Color);

    // 判断棋子是否有根（有己方保护）
    bool IsPieceRooted(int32 X, int32 Y, EChessColor Color);
    // 评估吃子后的安全性（是否会被反吃）
    bool IsCaptureSafe(const FChessMove2P& Move, EChessColor AttackerColor);
    // 判断棋子是否孤军深入（无支援）
    bool IsPieceIsolated(int32 X, int32 Y, EChessColor Color);
    // 检查核心棋子是否被攻击
    bool IsKeyPieceUnderAttack(EChessColor Color);
    // 检查是否是马的好位置
    bool IsGoodHorsePosition(int32 X, int32 Y, EChessColor Color);
    // 检查是否是炮的威胁位置
    bool IsThreateningPaoPosition(int32 X, int32 Y, EChessColor Color);
    // 检查面对面将军是否有风险
    bool IsRiskyFaceToFaceCheck(const FChessMove2P& Move, EChessColor Color);

    TWeakObjectPtr<AChesses> FindAttacker(FIntPoint Position, EChessColor AttackerColor);


    // Zobrist哈希值生成（用于棋盘局面唯一标识）
    uint64 GenerateZobristKey();

    // α-β剪枝搜索核心函数
    int32 AlphaBetaSearch(int32 Depth, int32 Alpha, int32 Beta, EChessColor CurrentColor, bool IsMaximizingPlayer);

    // 迭代加深搜索
    int32 IterativeDeepeningSearch(int32 MaxDepth, EChessColor AiColor);

    // 评估函数（优化：新增安全性/策略维度）
    int32 EvaluateBoard(EChessColor AiColor);

    // 走法排序（重构：优先安全/有收益走法）
    void SortMoves(TArray<FChessMove2P>& Moves, EChessColor Color);

    // 获取游戏阶段
    EChessGamePhase GetGamePhase();

    // 获取棋子基础价值（随游戏阶段调整）
    int32 GetPieceBaseValue(EChessType PieceType, EChessGamePhase Phase, EChessColor AiColor);

    // 获取棋子位置价值
    int32 GetPiecePositionValue(EChessType PieceType, EChessColor Color, int32 X, int32 Y);

    // 校验走棋后是否自将
    bool IsInCheckAfterMove(const FChessMove2P& Move, EChessColor SelfColor);

    // 校验下一步是否能吃掉对方将
    bool CanCaptureGeneralInNextStep(EChessColor SelfColor);

    // 检查是否将军
    bool IsInCheck(EChessColor Color);

    // 缓存将/帅位置
    bool GetKingPosition(EChessColor Color, int32& OutX, int32& OutY);

    // 更新置换表
    void UpdateTranspositionTable(int32 Depth, int32 BestValue, FChessMove2P BestMove, int32 Alpha, int32 Beta);

    // 清空置换表
    void ClearTranspositionTable() { TranspositionTable.Empty(); }

    int32 GetSearchDepth(EAI2PDifficulty Difficulty) 
    {
        switch (Difficulty)
        {
        case EAI2PDifficulty::Easy:
            return 3;
        case EAI2PDifficulty::Normal:
            return 5;
        case EAI2PDifficulty::Hard:
            return 4;
        default:
            return 4;
        }
    }

    FString MoveToString(const FChessMove2P& Move) const;

    FChessMove2P StringToMove(const FString& MoveString) const;

    TArray<FString> GetValidMovesAsStrings(EChessColor Color) const;

    FString GetCurrentBoardFEN() const;

    FString GetPieceFENChar(EChessType PieceType, EChessColor Color) const;

    // === 调整：权重常量（强化防守/进攻组织）===
    static const int32 VALUE_JIANG = 10000;    // 将/帅
    static const int32 VALUE_CHE = 900;        // 车
    static const int32 VALUE_MA = 450;         // 马
    static const int32 VALUE_PAO = 500;        // 炮
    static const int32 VALUE_BING = 100;       // 兵（未过河）
    static const int32 VALUE_BING_CROSSED = 200; // 兵（过河）
    static const int32 VALUE_SHI = 200;        // 士
    static const int32 VALUE_XIANG = 200;      // 象

    // 防守权重
    static const int32 BONUS_BLOCK_LETHAL_ATTACK = 1200;  // 拦截致命进攻奖励（最高优先级）
    static const int32 BONUS_FILL_DEFENSE_WEAKNESS = 500; // 补位防守漏洞奖励
    static const int32 PENALTY_DEFENSE_WEAK = -800;       // 防守弱点惩罚
    static const int32 BONUS_ACTIVE_DEFENSE = 200;        // 主动防守（卡位）奖励

    // 进攻权重
    static const int32 BONUS_ATTACK_SYNERGY = 1000;       // 进攻协同奖励
    static const int32 BONUS_CONTROL_KEY_POINT = 700;     // 控关键点位奖励
    static const int32 BONUS_ATTACK_WEAKNESS = 800;       // 攻击对方弱点奖励
    static const int32 BONUS_CHECK = 1850;                // 将军奖励
    static const int32 BONUS_SAFE_CAPTURE = 400;          // 安全吃子奖励（进一步降低）

    // 战术奖励权重（核心，高于普通攻防）
    static const int32 BONUS_TACTIC_EXECUTE = 2000;       // 执行战术奖励
    static const int32 BONUS_TACTIC_PREPARE = 1000;       // 战术铺垫奖励
    static const int32 BONUS_WOCAOMA = 2000;              // 卧槽马战术额外奖励
    static const int32 BONUS_CHENDIPAO = 1800;            // 沉底炮战术额外奖励
    static const int32 BONUS_ZHONGLUTUPO = 1500;          // 中路突破战术额外奖励

    // 通用权重
    static const int32 BONUS_ROOTED_PIECE = 100;          // 有根棋子奖励
    static const int32 PENALTY_ISOLATED_PIECE = -150;     // 孤军深入惩罚
    static const int32 PENALTY_IN_CHECK = -10000;         // 被将军惩罚
    static const int32 PENALTY_SUICIDAL_MOVE = -1000;     // 送子惩罚
    static const int32 BONUS_PROTECTED_PIECE = 200;       // 有保护奖励
};