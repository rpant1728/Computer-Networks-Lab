// Network topology
//
//       n0 ----------- n1
//            1 Mbps
//             10 ms
//
// Flow from n0 to n1 using BulkSendApplication and 5 constant bit rate sources.
// Measure congestion window size changes, dropped tcp packets and cumulative bytes transferred across tcp variants. 

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/traffic-control-module.h"


using namespace ns3;

uint32_t packetDropCount = 0;

NS_LOG_COMPONENT_DEFINE("Compare TCP variants");

static void CwndTracer(Ptr<OutputStreamWrapper>stream, uint32_t oldval, uint32_t newval){
	*stream->GetStream() << Simulator::Now().GetSeconds() << "," << newval << std::endl;
}

static void TraceCwnd(std::string cwndTrFileName){
	AsciiTraceHelper ascii;
	if(cwndTrFileName.compare("") == 0){
		NS_LOG_DEBUG("No trace file for cwnd provided");
		return;
	}
	else{
		Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(cwndTrFileName.c_str());
		Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndTracer, stream));
	}
}

// static void packetDropSample(Ptr<OutputStreamWrapper> stream, QueueDiscContainer &queues){
// 	Ptr<QueueDisc> queueDisk = queues.Get(0);
// 	packetDropCount = queueDisk->stats.GetNDroppedPackets;
//   	*stream->GetStream() << Simulator::Now().GetSeconds() << ","  << packetDropCount << std::endl;
// 	Simulator::Schedule(MilliSeconds(1), &packetDropSample, stream);
// }

static void packetDropSample(Ptr<OutputStreamWrapper> stream, Ptr<FlowMonitor> flowMonitor){
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();
	packetDropCount = stats[1].lostPackets;
  	*stream->GetStream() << Simulator::Now().GetSeconds() << ","  << packetDropCount << std::endl;
	Simulator::Schedule(MilliSeconds(1), &packetDropSample, stream, flowMonitor);
}

static void packetDropIncrement(Ptr<const Packet> p){
  	NS_LOG_UNCOND("Packet drop at " << Simulator::Now().GetSeconds());
	packetDropCount++;
}

static void TxTotalBytesSample(Ptr<OutputStreamWrapper> stream, std::vector <Ptr<PacketSink>> sinks){
	uint32_t totalBytes = 0;
	for (uint32_t it = 0; it < 6; it++){
		totalBytes += sinks[it]->GetTotalRx();
	}
	*stream->GetStream() << Simulator::Now().GetSeconds() << ","  << totalBytes << std::endl;
	Simulator::Schedule(Seconds(0.001), &TxTotalBytesSample, stream, sinks);
}

int main(int argc, char *argv[]){
	uint32_t maxBytes = 0;
  	uint32_t pktSize = 1458;        //in bytes. 1458 to prevent fragments
	std::string transport_prot = "TCPNewReno";
	std::string cwndTrFileName = "Cwnd.tr";
    std::string pktDropFileName = "PktDrop.tr";
    std::string bytesTxFileName = "BytesTx.tr";

	// Allow the user to override any of the defaults at run-time, via command-line arguments
	CommandLine cmd;
	cmd.AddValue("transport_prot", "Transport protocol to use: TcpNewReno, TcpHybla, TcpVegas, TcpScalable, TcpWestwood", transport_prot);
	cmd.Parse(argc, argv);

	NS_LOG_INFO("Create nodes.");
	NodeContainer nodes;
	nodes.Create(2);

	NS_LOG_INFO("Create channels."); 
	// LogComponentEnable ("BulkSendApplication", LOG_LEVEL_ALL); 
    // LogComponentEnable ("OnOffApplication", LOG_LEVEL_ALL); 

	// Explicitly create the point-to-point link required by the topology(shown above).
	PointToPointHelper pointToPoint;
	// TrafficControlHelper tch;
	// tch.Uninstall()
	pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
	pointToPoint.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));

	NetDeviceContainer devices;
	devices = pointToPoint.Install(nodes);
	// TrafficControlHelper tch; 
	// tch.Uninstall (devices); 

	if(transport_prot.compare("TcpNewReno") == 0){ 
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
	}
	else if(transport_prot.compare("TcpHybla") == 0){
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
	}
	else if(transport_prot.compare("TcpWestwood") == 0){
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
	}
	else if(transport_prot.compare("TcpScalable") == 0){
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpScalable::GetTypeId()));
	}
	else if(transport_prot.compare("TcpVegas") == 0){
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpVegas::GetTypeId()));
	}
	else {
		Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
	}

    // Set error rate to cover losses other than buffer loss
	// Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
	// em->SetAttribute("ErrorRate", DoubleValue(0.00001));
	// devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

	// Install the internet stack on the nodes
	InternetStackHelper internet;
	internet.Install(nodes);

	TrafficControlHelper tch;
  	tch.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize", StringValue ("50p"));
	QueueDiscContainer qdiscs = tch.Install (devices);
	//tch.Uninstall(devices);
	
	// We've got the "hardware" in place. Now we need to add IP addresses.
	NS_LOG_INFO("Assign IP Addresses.");
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer i = ipv4.Assign(devices);

	NS_LOG_INFO("Create Applications.");

	uint16_t port = 50000;
	BulkSendHelper source("ns3::TcpSocketFactory",	InetSocketAddress(i.GetAddress(1), port));
	// Set the amount of data to send in bytes.  Zero is unlimited.
  	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(pktSize));
	source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
  	source.SetAttribute("SendSize", UintegerValue(pktSize));
	ApplicationContainer sourceApps = source.Install(nodes.Get(0));
	sourceApps.Start(MilliSeconds(0));
	sourceApps.Stop(MilliSeconds(18000));

	// Create a PacketSinkApplication and install it on node 1
	PacketSinkHelper sink("ns3::TcpSocketFactory",	InetSocketAddress(Ipv4Address::GetAny(), port));
	ApplicationContainer sinkApp = sink.Install(nodes.Get(1));
	sinkApp.Start(MilliSeconds(0));
	sinkApp.Stop(MilliSeconds(18000));

	// Add 5 constant bit rate sources
	uint16_t cbr_port = 10;
	std::vector<int16_t> start_times{2000, 4000, 6000, 8000, 10000};
	std::vector<int16_t>  stop_times{18000, 18000, 12000, 14000, 16000};
	std::vector<Ptr<PacketSink>> sinks(6);

	sinks[0] = DynamicCast<PacketSink>(sinkApp.Get(0));

	for(uint16_t it = 0; it < 5; it++) {
        // Create ith constant bit source
		OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(i.GetAddress(0), cbr_port+it)));
		onoff.SetConstantRate(DataRate("300Kbps"), pktSize);
		ApplicationContainer apps = onoff.Install(nodes.Get(0));
		apps.Start(MilliSeconds(start_times[it]));
		apps.Stop(MilliSeconds(stop_times[it]));

		// Create ith packet sink to receive above packets
		PacketSinkHelper sink("ns3::UdpSocketFactory",	Address(InetSocketAddress(Ipv4Address::GetAny(), cbr_port+it)));
		apps = sink.Install(nodes.Get(1));
		apps.Start(MilliSeconds(start_times[it]));
		apps.Stop(MilliSeconds(stop_times[it]));  
	
		sinks[it+1] = DynamicCast<PacketSink>(apps.Get(0));
	}

	// Now, do the actual simulation.
	NS_LOG_INFO("Run Simulation.");
	
    // Enable congestion window sampler
    Simulator::Schedule(Seconds(0.00001), &TraceCwnd, transport_prot+cwndTrFileName);
	
    
	Ptr<FlowMonitor> flowMonitor;
	FlowMonitorHelper flowHelper;
	flowMonitor = flowHelper.InstallAll();

	AsciiTraceHelper ascii;

	// Attach packet drop incrementer function
    devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&packetDropIncrement));
	Ptr<OutputStreamWrapper> packetDropStream = ascii.CreateFileStream(transport_prot + pktDropFileName);
	// Schedule first instance of sampler function, then it will schedule itself
	Simulator::Schedule(MilliSeconds(1), &packetDropSample, packetDropStream, flowMonitor);

	Ptr<OutputStreamWrapper> TxTotalByteStream = ascii.CreateFileStream(transport_prot + bytesTxFileName);
	Simulator::Schedule(MilliSeconds(1), &TxTotalBytesSample, TxTotalByteStream ,sinks);
	
	
	
	Simulator::Stop(MilliSeconds(18000));
	Simulator::Run();

	
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();
	std::cout << std::endl << "*** Flow monitor statistics ***" << std::endl;
	std::cout << "  Tx Packets/Bytes:   " << stats[1].txPackets
			<< " / " << stats[1].txBytes << std::endl;
	std::cout << "  Offered Load: " << stats[1].txBytes * 8.0 / (stats[1].timeLastTxPacket.GetSeconds () - stats[1].timeFirstTxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
	std::cout << "  Rx Packets/Bytes:   " << stats[1].rxPackets
			<< " / " << stats[1].rxBytes << std::endl;
	uint32_t packetsDroppedByQueueDisc = 0;
	uint64_t bytesDroppedByQueueDisc = 0;
	if (stats[1].packetsDropped.size () > Ipv4FlowProbe::DROP_QUEUE_DISC)
	{
		std::cout << stats[1].packetsDropped.size () << std::endl;
		packetsDroppedByQueueDisc = stats[1].packetsDropped[Ipv4FlowProbe::DROP_QUEUE_DISC];
		bytesDroppedByQueueDisc = stats[1].bytesDropped[Ipv4FlowProbe::DROP_QUEUE_DISC];
	}
	std::cout << "  Packets/Bytes Dropped by Queue Disc:   " << packetsDroppedByQueueDisc
			<< " / " << bytesDroppedByQueueDisc << std::endl;
	uint32_t packetsDroppedByNetDevice = 0;
	uint64_t bytesDroppedByNetDevice = 0;
	if (stats[1].packetsDropped.size () > Ipv4FlowProbe::DROP_QUEUE)
	{
		std::cout << stats[1].packetsDropped.size () << std::endl;
		packetsDroppedByNetDevice = stats[1].packetsDropped[Ipv4FlowProbe::DROP_QUEUE];
		bytesDroppedByNetDevice = stats[1].bytesDropped[Ipv4FlowProbe::DROP_QUEUE];
	}
	std::cout << "  Packets/Bytes Dropped by NetDevice:   " << packetsDroppedByNetDevice
			<< " / " << bytesDroppedByNetDevice << std::endl;
	std::cout << "  Throughput: " << stats[1].rxBytes * 8.0 / (stats[1].timeLastRxPacket.GetSeconds () - stats[1].timeFirstRxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
	std::cout << "  Mean delay:   " << stats[1].delaySum.GetSeconds () / stats[1].rxPackets << std::endl;
	std::cout << "  Mean jitter:   " << stats[1].jitterSum.GetSeconds () / (stats[1].rxPackets - 1) << std::endl;

  	for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i){
		std::cout << " --------------------------------- " << std::endl;
		std::cout << "Flow Id: " << i->first << std::endl;
		std::cout << "Tx Bytes: " << i->second.txBytes  << std::endl;
		std::cout << "Drop Packet Count: " << i->second.lostPackets << std::endl;
	}

	// flowMonitor->SerializeToXmlFile("abc.xml", true, true);

	Simulator::Destroy();
	NS_LOG_INFO("Done.");
}
