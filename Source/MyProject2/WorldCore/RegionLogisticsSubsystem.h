#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RegionLogisticsTypes.h"
#include "RegionLogisticsSubsystem.generated.h"

class UWorldStateSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRegionLogisticsChanged, int64, NewRevision);

/**
 * Owns route/infrastructure metadata and calculates logistics costs.
 * Physical bridges, sleds, ramps and ropeways remain owned by their existing actors/components.
 * Region resources and environmental risk are calculated from UWorldStateSubsystem on demand.
 */
UCLASS()
class MYPROJECT2_API URegionLogisticsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category="Region|Logistics")
    FIntPoint GetRegionCoordinateForWorldLocation(const FVector& WorldLocation) const;

    /** Deterministic IDs let existing actors/Blueprints re-register placed infrastructure after load. */
    UFUNCTION(BlueprintPure, Category="Region|Logistics")
    static FGuid MakeStableNodeId(FName StableNodeKey);

    UFUNCTION(BlueprintPure, Category="Region|Logistics")
    static FGuid MakeStableLinkId(FName StableLinkKey);

    UFUNCTION(BlueprintCallable, Category="Region|Logistics")
    bool RegisterNamedNode(FName StableNodeKey, const FVector& WorldLocation, FName NodeTag, bool bEnabled = true);

    UFUNCTION(BlueprintCallable, Category="Region|Logistics")
    bool RemoveNamedNode(FName StableNodeKey);

    /**
     * Register/update a road, bridge, ramp, sled route or ropeway segment.
     * Reusing StableLinkKey updates that segment; equal StartNodeKey/EndNodeKey values join a shared junction.
     */
    UFUNCTION(BlueprintCallable, Category="Region|Logistics")
    bool RegisterInfrastructureSegment(
        FName StableLinkKey,
        FName StartNodeKey,
        const FVector& StartLocation,
        FName EndNodeKey,
        const FVector& EndLocation,
        ELogisticsLinkType Type,
        float MaxPayloadKg,
        float Condition01 = 1.0f,
        float Risk01 = 0.0f,
        bool bBidirectional = true,
        bool bOperational = true);

    UFUNCTION(BlueprintCallable, Category="Region|Logistics")
    bool SetInfrastructureStatus(FName StableLinkKey, bool bOperational, float Condition01, float Risk01);

    UFUNCTION(BlueprintCallable, Category="Region|Logistics")
    bool UnregisterInfrastructureSegment(FName StableLinkKey);

    UFUNCTION(BlueprintCallable, Category="Region|Logistics|Route")
    bool FindLowestCostRouteByKey(FName StartNodeKey, FName DestinationNodeKey, float PayloadMassKg, FRegionRouteResult& OutRoute) const;

    UFUNCTION(BlueprintCallable, Category="Region|Survey")
    bool QueryRegionSurvey(const FIntPoint& RegionCoord, FRegionSurvey& OutSurvey) const;

    UFUNCTION(BlueprintCallable, Category="Region|Logistics|Save")
    bool ExportSnapshot(FRegionLogisticsSnapshot& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category="Region|Logistics|Save")
    bool ValidateSnapshot(const FRegionLogisticsSnapshot& Snapshot) const;

    UFUNCTION(BlueprintCallable, Category="Region|Logistics|Save")
    bool ImportSnapshot(const FRegionLogisticsSnapshot& Snapshot);

    UFUNCTION(BlueprintPure, Category="Region|Logistics")
    int64 GetRevision() const { return Revision; }

    UPROPERTY(BlueprintAssignable, Category="Region|Logistics")
    FOnRegionLogisticsChanged OnLogisticsChanged;

    /** Number of canonical world chunks per region axis; floor-division supports negative coordinates. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Region|Configuration", meta=(ClampMin="1", ClampMax="128"))
    int32 ChunksPerRegion = 8;

protected:
    int32 FindNodeIndexById(const FGuid& NodeId) const;
    int32 FindNodeIndexByKey(FName StableNodeKey) const;
    int32 FindLinkIndexByKey(FName StableLinkKey) const;
    void RemoveOrphanNode(const FGuid& NodeId);
    void MarkChanged();
    bool IsValidNodeData(FName StableNodeKey, const FVector& WorldLocation) const;
    float GetLinkBaseSpeedMetersPerSecond(ELogisticsLinkType Type) const;
    float GetEnvironmentalRiskCached(const FVector& Location, TMap<FIntPoint, float>& RiskCache) const;
    bool ComputeTraversalCost(
        const FRegionLogisticsLink& Link,
        const FRegionLogisticsNode& FromNode,
        const FRegionLogisticsNode& ToNode,
        float PayloadMassKg,
        TMap<FIntPoint, float>& RiskCache,
        float& OutDistanceMeters,
        float& OutSeconds,
        float& OutRisk01) const;

    UPROPERTY(Transient)
    TArray<FRegionLogisticsNode> Nodes;

    UPROPERTY(Transient)
    TArray<FRegionLogisticsLink> Links;

    UPROPERTY(Transient)
    int64 Revision = 0;

    static constexpr int32 MaxNodes = 2048;
    static constexpr int32 MaxLinks = 8192;
};
