/**
 * @file DynamicLayout.cpp
 *
 * @ingroup Simulator/Layouts
 * 
 * @brief The DynamicLayout class defines the layout of neurons in neural networks
 */

#include "DynamicLayout.h"
#include "ParameterManager.h"
#include "ParseParamError.h"
#include "Util.h"

// TODO: Neither the constructor nor the destructor are needed here, right?
DynamicLayout::DynamicLayout() : Layout() {}

DynamicLayout::~DynamicLayout() {}

///  Prints out all parameters to logging file.
///  Registered to OperationManager as Operation::printParameters
void DynamicLayout::printParameters() const {
	Layout::printParameters();
	LOG4CPLUS_DEBUG(fileLogger_, "\n\tLayout type: Dynamic Layout" << std::endl
	                << "\tfraction endogenously active:" << fractionEndogenouslyActive_ << std::endl
	                << "\tfraction excitatory:" << fractionExcitatory_ << std::endl << std::endl);
}

///  Creates a randomly ordered distribution with the specified numbers of neuron types.
///
///  @param  numVertices number of the vertices to have in the type map.
void DynamicLayout::generateVertexTypeMap(int numVertices) {
	LOG4CPLUS_DEBUG(fileLogger_, "\nInitializing vertex type map..." << std::endl);

	// Populate vertexTypeMap_ with EXC
	std::fill_n(vertexTypeMap_, numVertices, vertexType::EXC);

	// for (int i = 0; i < numVertices; i++) {
	//    vertexTypeMap_[i] = EXC;
	// }

	int numExcitatory = static_cast<int>(fractionExcitatory_ * numVertices + 0.5);
	int numInhibitory = numVertices - numExcitatory;

	LOG4CPLUS_DEBUG(fileLogger_, "\nVERTEX TYPE MAP" << std::endl
	                << "\tTotal vertices: " << numVertices << std::endl
	                << "\tInhibitory Neurons: " << numInhibitory << std::endl
	                << "\tExcitatory Neurons: " << numExcitatory << std::endl);

	LOG4CPLUS_INFO(fileLogger_, "Randomly selecting inhibitory neurons...");

	auto rgInhibitoryLayout = new int[numInhibitory];

	for (int i = 0; i < numInhibitory; i++) rgInhibitoryLayout[i] = i;

	for (int i = numInhibitory; i < numVertices; i++) {
		int j = static_cast<int>(initRNG() * numVertices);
		if (j < numInhibitory) rgInhibitoryLayout[j] = i;
	}

	for (int i = 0; i < numInhibitory; i++) vertexTypeMap_[rgInhibitoryLayout[i]] = vertexType::INH;
	delete[] rgInhibitoryLayout;

	LOG4CPLUS_INFO(fileLogger_, "Done initializing vertex type map");
}

///  Populates the starter map.
///  Selects numEndogenouslyActiveNeurons_ excitatory neurons
///  and converts them into starter neurons.
///
///  @param  numVertices number of vertices to have in the map.
void DynamicLayout::initStarterMap(const int numVertices) {
	Layout::initStarterMap(numVertices);

	numEndogenouslyActiveNeurons_ = static_cast<uint32_t>(fractionEndogenouslyActive_ * numVertices + 0.5);
	BGSIZE startersAllocated = 0;

	LOG4CPLUS_DEBUG(fileLogger_, "\nNEURON STARTER MAP" << std::endl
	                << "\tTotal Neurons: " << numVertices << std::endl
	                << "\tStarter Neurons: " << numEndogenouslyActiveNeurons_
	                << std::endl);

	// randomly set neurons as starters until we've created enough
	while (startersAllocated < numEndogenouslyActiveNeurons_) {
		// Get a random neuron ID
		int i = static_cast<int>(initRNG.inRange(0, numVertices));

		// If the neuron at that index is excitatory and not already in the
		// starter map, add an entry.
		if (vertexTypeMap_[i] == vertexType::EXC && !starterMap_[i]) {
			starterMap_[i] = true;
			startersAllocated++;
			LOG4CPLUS_DEBUG(fileLogger_, "Allocated EA neuron at random index [" << i << "]" << std::endl;);
		}
	}

	LOG4CPLUS_INFO(fileLogger_, "Done randomly initializing starter map");
}

/// Load member variables from configuration file. Registered to OperationManager as Operations::op::loadParameters
void DynamicLayout::loadParameters() {
	// Get the file paths for the Neuron lists from the configuration file
	std::string activeNListFilePath;
	std::string inhibitoryNListFilePath;
	if (!ParameterManager::getInstance().getStringByXpath("//LayoutFiles/activeNListFileName/text()",
	                                                      activeNListFilePath)) {
		throw std::runtime_error("In Layout::loadParameters() Endogenously "
			"active neuron list file path wasn't found and will not be initialized");
	}
	if (!ParameterManager::getInstance().getStringByXpath("//LayoutFiles/inhNListFileName/text()",
	                                                      inhibitoryNListFilePath)) {
		throw std::runtime_error("In Layout::loadParameters() "
			"Inhibitory neuron list file path wasn't found and will not be initialized");
	}

	// Initialize Neuron Lists based on the data read from the xml files
	if (!ParameterManager::getInstance().getIntVectorByXpath(activeNListFilePath, "A", endogenouslyActiveNeuronList_)) {
		throw std::runtime_error("In Layout::loadParameters() "
			"Endogenously active neuron list file wasn't loaded correctly"
			"\n\tfile path: " + activeNListFilePath);
	}
	numEndogenouslyActiveNeurons_ = endogenouslyActiveNeuronList_.size();
	if (!ParameterManager::getInstance().getIntVectorByXpath(inhibitoryNListFilePath, "I", inhibitoryNeuronLayout_)) {
		throw std::runtime_error("In Layout::loadParameters() "
			"Inhibitory neuron list file wasn't loaded correctly."
			"\n\tfile path: " + inhibitoryNListFilePath);
	}
}

///  Returns the type of synapse at the given coordinates
///
///  @param    srcVertex  integer that points to a Neuron in the type map as a source.
///  @param    destVertex integer that points to a Neuron in the type map as a destination.
///  @return type of the synapse.
edgeType DynamicLayout::edgType(const int srcVertex, const int destVertex) {
	if (vertexTypeMap_[srcVertex] == vertexType::INH && vertexTypeMap_[destVertex] == vertexType::INH) return
		edgeType::II;
	if (vertexTypeMap_[srcVertex] == vertexType::INH && vertexTypeMap_[destVertex] == vertexType::EXC) return
		edgeType::IE;
	if (vertexTypeMap_[srcVertex] == vertexType::EXC && vertexTypeMap_[destVertex] == vertexType::INH) return
		edgeType::EI;
	if (vertexTypeMap_[srcVertex] == vertexType::EXC && vertexTypeMap_[destVertex] == vertexType::EXC) return
		edgeType::EE;

	return edgeType::ETYPE_UNDEF;
}
