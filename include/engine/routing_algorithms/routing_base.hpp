#ifndef OSRM_ENGINE_ROUTING_BASE_HPP
#define OSRM_ENGINE_ROUTING_BASE_HPP

#include "guidance/turn_bearing.hpp"
#include "guidance/turn_instruction.hpp"

#include "engine/algorithm.hpp"
#include "engine/datafacade.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "engine/search_engine_data.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{

namespace routing_algorithms
{
static constexpr bool FORWARD_DIRECTION = true;
static constexpr bool REVERSE_DIRECTION = false;
static constexpr bool DO_NOT_FORCE_LOOPS = false;

bool needsLoopForward(const PhantomNode &source_phantom, const PhantomNode &target_phantom);
bool needsLoopBackwards(const PhantomNode &source_phantom, const PhantomNode &target_phantom);

bool needsLoopForward(const PhantomNodes &phantoms);
bool needsLoopBackwards(const PhantomNodes &phantoms);

namespace detail
{
template <typename Algorithm>
void insertSourceInHeap(typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &heap,
                        const PhantomNode &phantom_node)
{
    if (phantom_node.IsValidForwardTarget())
    {
        heap.Insert(phantom_node.forward_segment_id.id,
                    -phantom_node.GetForwardWeightPlusOffset(),
                    {phantom_node.forward_segment_id.id, -phantom_node.GetForwardDuration()});
    }
    if (phantom_node.IsValidReverseTarget())
    {
        heap.Insert(phantom_node.reverse_segment_id.id,
                    -phantom_node.GetReverseWeightPlusOffset(),
                    {phantom_node.reverse_segment_id.id, -phantom_node.GetReverseDuration()});
    }
}

template <typename Algorithm>
void insertTargetInHeap(typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &heap,
                        const PhantomNode &phantom_node)
{
    if (phantom_node.IsValidForwardTarget())
    {
        heap.Insert(phantom_node.forward_segment_id.id,
                    phantom_node.GetForwardWeightPlusOffset(),
                    {phantom_node.forward_segment_id.id, phantom_node.GetForwardDuration()});
    }
    if (phantom_node.IsValidReverseTarget())
    {
        heap.Insert(phantom_node.reverse_segment_id.id,
                    phantom_node.GetReverseWeightPlusOffset(),
                    {phantom_node.reverse_segment_id.id, phantom_node.GetReverseDuration()});
    }
}

template <typename Algorithm>
void insertSourceInHeap(typename SearchEngineData<Algorithm>::QueryHeap &heap,
                        const PhantomNode &phantom_node)
{
    if (phantom_node.IsValidForwardSource())
    {
        heap.Insert(phantom_node.forward_segment_id.id,
                    -phantom_node.GetForwardWeightPlusOffset(),
                    phantom_node.forward_segment_id.id);
    }
    if (phantom_node.IsValidReverseSource())
    {
        heap.Insert(phantom_node.reverse_segment_id.id,
                    -phantom_node.GetReverseWeightPlusOffset(),
                    phantom_node.reverse_segment_id.id);
    }
}

template <typename Algorithm>
void insertTargetInHeap(typename SearchEngineData<Algorithm>::QueryHeap &heap,
                        const PhantomNode &phantom_node)
{
    if (phantom_node.IsValidForwardTarget())
    {
        heap.Insert(phantom_node.forward_segment_id.id,
                    phantom_node.GetForwardWeightPlusOffset(),
                    phantom_node.forward_segment_id.id);
    }
    if (phantom_node.IsValidReverseTarget())
    {
        heap.Insert(phantom_node.reverse_segment_id.id,
                    phantom_node.GetReverseWeightPlusOffset(),
                    phantom_node.reverse_segment_id.id);
    }
}
} // namespace detail

inline void insertTargetInHeap(typename SearchEngineData<mld::Algorithm>::ManyToManyQueryHeap &heap,
                               const PhantomNode &phantom_node)
{
    detail::insertTargetInHeap<mld::Algorithm>(heap, phantom_node);
}
inline void insertTargetInHeap(typename SearchEngineData<ch::Algorithm>::ManyToManyQueryHeap &heap,
                               const PhantomNode &phantom_node)
{
    detail::insertTargetInHeap<ch::Algorithm>(heap, phantom_node);
}
inline void insertTargetInHeap(typename SearchEngineData<mld::Algorithm>::QueryHeap &heap,
                               const PhantomNode &phantom_node)
{
    detail::insertTargetInHeap<mld::Algorithm>(heap, phantom_node);
}
inline void insertTargetInHeap(typename SearchEngineData<ch::Algorithm>::QueryHeap &heap,
                               const PhantomNode &phantom_node)
{
    detail::insertTargetInHeap<ch::Algorithm>(heap, phantom_node);
}
inline void insertSourceInHeap(typename SearchEngineData<mld::Algorithm>::ManyToManyQueryHeap &heap,
                               const PhantomNode &phantom_node)
{
    detail::insertSourceInHeap<mld::Algorithm>(heap, phantom_node);
}
inline void insertSourceInHeap(typename SearchEngineData<ch::Algorithm>::ManyToManyQueryHeap &heap,
                               const PhantomNode &phantom_node)
{
    detail::insertSourceInHeap<ch::Algorithm>(heap, phantom_node);
}
inline void insertSourceInHeap(typename SearchEngineData<mld::Algorithm>::QueryHeap &heap,
                               const PhantomNode &phantom_node)
{
    detail::insertSourceInHeap<mld::Algorithm>(heap, phantom_node);
}
inline void insertSourceInHeap(typename SearchEngineData<ch::Algorithm>::QueryHeap &heap,
                               const PhantomNode &phantom_node)
{
    detail::insertSourceInHeap<ch::Algorithm>(heap, phantom_node);
}

template <typename Heap>
void insertNodesInHeaps(Heap &forward_heap, Heap &reverse_heap, const PhantomNodes &nodes)
{
    insertSourceInHeap(forward_heap, nodes.source_phantom);
    insertTargetInHeap(reverse_heap, nodes.target_phantom);
}

template <typename Algorithm>
void insertSourceInHeap(typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &heap,
                        const PhantomNode &phantom_node)
{
    if (phantom_node.IsValidForwardSource())
    {
        heap.Insert(phantom_node.forward_segment_id.id,
                    -phantom_node.GetForwardWeightPlusOffset(),
                    {phantom_node.forward_segment_id.id, -phantom_node.GetForwardDuration()});
    }
    if (phantom_node.IsValidReverseSource())
    {
        heap.Insert(phantom_node.reverse_segment_id.id,
                    -phantom_node.GetReverseWeightPlusOffset(),
                    {phantom_node.reverse_segment_id.id, -phantom_node.GetReverseDuration()});
    }
}

template <typename FacadeT>
void annotatePath(const FacadeT &facade,
                  const PhantomNodes &phantom_node_pair,
                  const std::vector<NodeID> &unpacked_nodes,
                  const std::vector<EdgeID> &unpacked_edges,
                  std::vector<PathData> &unpacked_path)
{
    BOOST_ASSERT(!unpacked_nodes.empty());
    BOOST_ASSERT(unpacked_nodes.size() == unpacked_edges.size() + 1);

    const auto source_node_id = unpacked_nodes.front();
    const auto target_node_id = unpacked_nodes.back();
    const bool start_traversed_in_reverse =
        phantom_node_pair.source_phantom.forward_segment_id.id != source_node_id;
    const bool target_traversed_in_reverse =
        phantom_node_pair.target_phantom.forward_segment_id.id != target_node_id;

    BOOST_ASSERT(phantom_node_pair.source_phantom.forward_segment_id.id == source_node_id ||
                 phantom_node_pair.source_phantom.reverse_segment_id.id == source_node_id);
    BOOST_ASSERT(phantom_node_pair.target_phantom.forward_segment_id.id == target_node_id ||
                 phantom_node_pair.target_phantom.reverse_segment_id.id == target_node_id);

    // datastructures to hold extracted data from geometry
    std::vector<NodeID> id_vector;
    std::vector<SegmentWeight> weight_vector;
    std::vector<SegmentDuration> duration_vector;
    std::vector<DatasourceID> datasource_vector;

    const auto get_segment_geometry = [&](const auto geometry_index) {
        const auto copy = [](auto &vector, const auto range) {
            vector.resize(range.size());
            std::copy(range.begin(), range.end(), vector.begin());
        };

        if (geometry_index.forward)
        {
            copy(id_vector, facade.GetUncompressedForwardGeometry(geometry_index.id));
            copy(weight_vector, facade.GetUncompressedForwardWeights(geometry_index.id));
            copy(duration_vector, facade.GetUncompressedForwardDurations(geometry_index.id));
            copy(datasource_vector, facade.GetUncompressedForwardDatasources(geometry_index.id));
        }
        else
        {
            copy(id_vector, facade.GetUncompressedReverseGeometry(geometry_index.id));
            copy(weight_vector, facade.GetUncompressedReverseWeights(geometry_index.id));
            copy(duration_vector, facade.GetUncompressedReverseDurations(geometry_index.id));
            copy(datasource_vector, facade.GetUncompressedReverseDatasources(geometry_index.id));
        }
    };

    auto node_from = unpacked_nodes.begin(), node_last = std::prev(unpacked_nodes.end());
    for (auto edge = unpacked_edges.begin(); node_from != node_last; ++node_from, ++edge)
    {
        const auto &edge_data = facade.GetEdgeData(*edge);
        const auto turn_id = edge_data.turn_id; // edge-based graph edge index
        const auto node_id = *node_from;        // edge-based graph node index
        const auto name_index = facade.GetNameIndex(node_id);
        const bool is_segregated = facade.IsSegregated(node_id);
        const auto turn_instruction = facade.GetTurnInstructionForEdgeID(turn_id);
        const extractor::TravelMode travel_mode = facade.GetTravelMode(node_id);
        const auto classes = facade.GetClassData(node_id);

        const auto geometry_index = facade.GetGeometryIndex(node_id);
        get_segment_geometry(geometry_index);

        BOOST_ASSERT(id_vector.size() > 0);
        BOOST_ASSERT(datasource_vector.size() > 0);
        BOOST_ASSERT(weight_vector.size() + 1 == id_vector.size());
        BOOST_ASSERT(duration_vector.size() + 1 == id_vector.size());

        const bool is_first_segment = unpacked_path.empty();

        const std::size_t start_index =
            (is_first_segment ? ((start_traversed_in_reverse)
                                     ? weight_vector.size() -
                                           phantom_node_pair.source_phantom.fwd_segment_position - 1
                                     : phantom_node_pair.source_phantom.fwd_segment_position)
                              : 0);
        const std::size_t end_index = weight_vector.size();

        bool is_left_hand_driving = facade.IsLeftHandDriving(node_id);

        BOOST_ASSERT(start_index >= 0);
        BOOST_ASSERT(start_index < end_index);
        for (std::size_t segment_idx = start_index; segment_idx < end_index; ++segment_idx)
        {
            unpacked_path.push_back(
                PathData{*node_from,
                         id_vector[segment_idx + 1],
                         name_index,
                         is_segregated,
                         static_cast<EdgeWeight>(weight_vector[segment_idx]),
                         0,
                         static_cast<EdgeDuration>(duration_vector[segment_idx]),
                         0,
                         guidance::TurnInstruction::NO_TURN(),
                         {{0, INVALID_LANEID}, INVALID_LANE_DESCRIPTIONID},
                         travel_mode,
                         classes,
                         EMPTY_ENTRY_CLASS,
                         datasource_vector[segment_idx],
                         osrm::guidance::TurnBearing(0),
                         osrm::guidance::TurnBearing(0),
                         is_left_hand_driving});
        }
        BOOST_ASSERT(unpacked_path.size() > 0);
        if (facade.HasLaneData(turn_id))
            unpacked_path.back().lane_data = facade.GetLaneData(turn_id);

        const auto turn_duration = facade.GetDurationPenaltyForEdgeID(turn_id);
        const auto turn_weight = facade.GetWeightPenaltyForEdgeID(turn_id);

        unpacked_path.back().entry_class = facade.GetEntryClass(turn_id);
        unpacked_path.back().turn_instruction = turn_instruction;
        unpacked_path.back().duration_until_turn += turn_duration;
        unpacked_path.back().duration_of_turn = turn_duration;
        unpacked_path.back().weight_until_turn += turn_weight;
        unpacked_path.back().weight_of_turn = turn_weight;
        unpacked_path.back().pre_turn_bearing = facade.PreTurnBearing(turn_id);
        unpacked_path.back().post_turn_bearing = facade.PostTurnBearing(turn_id);
    }

    std::size_t start_index = 0, end_index = 0;
    const auto source_geometry_id = facade.GetGeometryIndex(source_node_id).id;
    const auto target_geometry = facade.GetGeometryIndex(target_node_id);
    const auto is_local_path = source_geometry_id == target_geometry.id && unpacked_path.empty();

    get_segment_geometry(target_geometry);

    if (target_traversed_in_reverse)
    {
        if (is_local_path)
        {
            start_index =
                weight_vector.size() - phantom_node_pair.source_phantom.fwd_segment_position - 1;
        }
        end_index =
            weight_vector.size() - phantom_node_pair.target_phantom.fwd_segment_position - 1;
    }
    else
    {
        if (is_local_path)
        {
            start_index = phantom_node_pair.source_phantom.fwd_segment_position;
        }
        end_index = phantom_node_pair.target_phantom.fwd_segment_position;
    }

    // Given the following compressed geometry:
    // U---v---w---x---y---Z
    //    s           t
    // s: fwd_segment 0
    // t: fwd_segment 3
    // -> (U, v), (v, w), (w, x)
    // note that (x, t) is _not_ included but needs to be added later.
    bool is_target_left_hand_driving = facade.IsLeftHandDriving(target_node_id);
    for (std::size_t segment_idx = start_index; segment_idx != end_index;
         (start_index < end_index ? ++segment_idx : --segment_idx))
    {
        BOOST_ASSERT(segment_idx < static_cast<std::size_t>(id_vector.size() - 1));
        BOOST_ASSERT(facade.GetTravelMode(target_node_id) > 0);
        unpacked_path.push_back(
            PathData{target_node_id,
                     id_vector[start_index < end_index ? segment_idx + 1 : segment_idx - 1],
                     facade.GetNameIndex(target_node_id),
                     facade.IsSegregated(target_node_id),
                     static_cast<EdgeWeight>(weight_vector[segment_idx]),
                     0,
                     static_cast<EdgeDuration>(duration_vector[segment_idx]),
                     0,
                     guidance::TurnInstruction::NO_TURN(),
                     {{0, INVALID_LANEID}, INVALID_LANE_DESCRIPTIONID},
                     facade.GetTravelMode(target_node_id),
                     facade.GetClassData(target_node_id),
                     EMPTY_ENTRY_CLASS,
                     datasource_vector[segment_idx],
                     guidance::TurnBearing(0),
                     guidance::TurnBearing(0),
                     is_target_left_hand_driving});
    }

    if (unpacked_path.size() > 0)
    {
        const auto source_weight = start_traversed_in_reverse
                                       ? phantom_node_pair.source_phantom.reverse_weight
                                       : phantom_node_pair.source_phantom.forward_weight;
        const auto source_duration = start_traversed_in_reverse
                                         ? phantom_node_pair.source_phantom.reverse_duration
                                         : phantom_node_pair.source_phantom.forward_duration;
        // The above code will create segments for (v, w), (w,x), (x, y) and (y, Z).
        // However the first segment duration needs to be adjusted to the fact that the source
        // phantom is in the middle of the segment. We do this by subtracting v--s from the
        // duration.

        // Since it's possible duration_until_turn can be less than source_weight here if
        // a negative enough turn penalty is used to modify this edge weight during
        // osrm-contract, we clamp to 0 here so as not to return a negative duration
        // for this segment.

        // TODO this creates a scenario where it's possible the duration from a phantom
        // node to the first turn would be the same as from end to end of a segment,
        // which is obviously incorrect and not ideal...
        unpacked_path.front().weight_until_turn =
            std::max(unpacked_path.front().weight_until_turn - source_weight, 0);
        unpacked_path.front().duration_until_turn =
            std::max(unpacked_path.front().duration_until_turn - source_duration, 0);
    }
}

EdgeDistance adjustPathDistanceToPhantomNodes(const std::vector<NodeID> &path,
                                              const PhantomNode &source_phantom,
                                              const PhantomNode &target_phantom,
                                              const EdgeDistance distance);

template <typename AlgorithmT>
InternalRouteResult extractRoute(const DataFacade<AlgorithmT> &facade,
                                 const EdgeWeight weight,
                                 const PhantomNodes &phantom_nodes,
                                 const std::vector<NodeID> &unpacked_nodes,
                                 const std::vector<EdgeID> &unpacked_edges)
{
    InternalRouteResult raw_route_data;
    raw_route_data.segment_end_coordinates = {phantom_nodes};

    // No path found for both target nodes?
    if (INVALID_EDGE_WEIGHT == weight)
    {
        return raw_route_data;
    }

    raw_route_data.shortest_path_weight = weight;
    raw_route_data.unpacked_path_segments.resize(1);
    raw_route_data.source_traversed_in_reverse.push_back(
        (unpacked_nodes.front() != phantom_nodes.source_phantom.forward_segment_id.id));
    raw_route_data.target_traversed_in_reverse.push_back(
        (unpacked_nodes.back() != phantom_nodes.target_phantom.forward_segment_id.id));

    annotatePath(facade,
                 phantom_nodes,
                 unpacked_nodes,
                 unpacked_edges,
                 raw_route_data.unpacked_path_segments.front());

    return raw_route_data;
}

template <typename FacadeT> EdgeDistance computeEdgeDistance(const FacadeT &facade, NodeID node_id)
{
    const auto geometry_index = facade.GetGeometryIndex(node_id);

    EdgeDistance total_distance = 0.0;

    auto geometry_range = facade.GetUncompressedForwardGeometry(geometry_index.id);
    for (auto current = geometry_range.begin(); current < geometry_range.end() - 1; ++current)
    {
        total_distance += util::coordinate_calculation::fccApproximateDistance(
            facade.GetCoordinateOfNode(*current), facade.GetCoordinateOfNode(*std::next(current)));
    }

    return total_distance;
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif // OSRM_ENGINE_ROUTING_BASE_HPP
