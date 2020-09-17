#include <iostream>
#include <memory>
#include <cassert>

#include <Environment.h>
#include <Model.h>
#include <Utils.h>

void buildOpenSANDModel(Model *mm, Environment *env);
void buildOpenSANDModelSimplified(Model *mm, Environment *env);

int main(int argc, char *argv[])
{
     if (argc == 3)
     {
          std::string type = argv[1];
          std::string path = argv[2];

          std::string modelId;

          if (type == "SAT")
          {
               modelId = "sat_mm";
          }
          else if (type == "GW")
          {
               modelId = "gw_mm";
          }
          else if (type == "ST")
          {
               modelId = "st_mm";
          }
          else
          {
               std::cerr << "Unknown entity type \"" + type + "\"" << std::endl;
               return EXIT_FAILURE;
          }

          path += "/" + modelId + ".xsd";

          auto env = new Environment();
          auto mm = new Model("1.0.0", modelId, "Gateway Model", "Describes Gateway Model.");
          mm->setEnvironment(env);

          buildOpenSANDModelSimplified(mm, env);

          if (!Utils::toXSD(mm, path))
          {
               std::cerr << "Error while saving DM." << std::endl;
               return EXIT_FAILURE;
          }

          delete mm;
          delete env;

          return EXIT_SUCCESS;
     }

     auto env = new Environment();
     auto mm = new Model("5.1.2", "model_test", "Gateway Model", "Describes Gateway Model.");
     mm->setEnvironment(env);

     buildOpenSANDModelSimplified(mm, env);

     // Saving and loading datas to and from configuration files.

     Environment *envLoaded = nullptr;
     Model *dmLoaded = nullptr;

     if (Utils::toXSD(mm, "Configurations/model.xsd"))
     {
          if (Utils::toXML(mm, "Configurations/model.xml"))
          {
               std::cout << "MM and DM saved successfully." << std::endl;
          }
          else
          {
               std::cerr << "Error while saving DM." << std::endl;
               return EXIT_FAILURE;
          }
     }
     else
     {
          std::cerr << "Error while saving DM." << std::endl;
          return EXIT_FAILURE;
     }

     if (Utils::fromXSD(&envLoaded, &dmLoaded, "Configurations/model.xsd"))
     {
          if (Utils::fromXML(&envLoaded, &dmLoaded, "Configurations/model.xml", "Configurations/model.xsd"))
          {
               std::cout << "MM and DM loaded successfully." << std::endl;
          }
          else
          {
               std::cerr << "Error while loading DM." << std::endl;
               return EXIT_FAILURE;
          }
     }
     else
     {
          std::cerr << "Error while loading DM." << std::endl;
          return EXIT_FAILURE;
     }

     if (Utils::toXSD(dmLoaded, "Configurations/copy.xsd"))
     {
          if (Utils::toXML(dmLoaded, "Configurations/copy.xml"))
          {
               std::cout << "MM (copy) and DM (copy) saved successfully." << std::endl;
          }
          else
          {
               std::cerr << "Error while saving DM (copy)." << std::endl;
               return EXIT_FAILURE;
          }
     }
     else
     {
          std::cerr << "Error while loading DM (copy)." << std::endl;
          return EXIT_FAILURE;
     }

     if (Utils::validate("Configurations/model.xsd", "Configurations/model.xml"))
     {
          std::cout << "OK" << std::endl;
     }
     if (Utils::validate("Configurations/copy.xsd", "Configurations/copy.xml"))
     {
          std::cout << "OK" << std::endl;
     }
     if (Utils::validate("Configurations/model.xsd", "Configurations/copy.xml"))
     {
          std::cout << "OK" << std::endl;
     }
     if (Utils::validate("Configurations/copy.xsd", "Configurations/model.xml"))
     {
          std::cout << "OK" << std::endl;
     }

     assert(env->isSame(envLoaded));
     assert(mm->isSame(dmLoaded));

     delete dmLoaded;
     delete envLoaded;

     delete mm;
     delete env;

     return EXIT_SUCCESS;
}

void buildOpenSANDModel(Model *mm, Environment *env)
{
     auto lanprotoE = env->addEnumType("lanproto", "LAN Protocol");
     lanprotoE->addValue("IP");
     lanprotoE->addValue("Ethernet");
     lanprotoE->addValue("ROHC");
     lanprotoE->addValue("PHS");

     auto damaAgentAlgorithmE = env->addEnumType("damaAgentAlgorithm", "DAMA Agent Algorithm");
     damaAgentAlgorithmE->addValue("Legacy");
     damaAgentAlgorithmE->addValue("RrmQos");

     auto simutypeE = env->addEnumType("simuType", "Simulation Type");
     simutypeE->addValue("None");
     simutypeE->addValue("File");
     simutypeE->addValue("Random");

     auto classnameE = env->addEnumType("className", "Class Name");
     classnameE->addValue("NM");
     classnameE->addValue("EF");
     classnameE->addValue("SIG");
     classnameE->addValue("AF");
     classnameE->addValue("BE");

     auto accesstypeE = env->addEnumType("accessType", "Access Type");
     accesstypeE->addValue("ACM");
     accesstypeE->addValue("VCM0");
     accesstypeE->addValue("VCM1");
     accesstypeE->addValue("VCM2");
     accesstypeE->addValue("VCM3");

     auto saalgoE = env->addEnumType("sa_algo", "SA Algorithm");
     saalgoE->addValue("DSA");
     saalgoE->addValue("CRDSA");

     auto categoryE = env->addEnumType("category", "Category");
     categoryE->addValue("Standard");
     categoryE->addValue("Premium");
     categoryE->addValue("Pro");
     categoryE->addValue("SVNO1");
     categoryE->addValue("SVNO2");
     categoryE->addValue("SVNO3");
     categoryE->addValue("SNO");

     auto attenuationtypeE = env->addEnumType("attenuationType", "Attenuation Type");
     attenuationtypeE->addValue("Ideal");
     attenuationtypeE->addValue("File");
     attenuationtypeE->addValue("On/Off");
     attenuationtypeE->addValue("Triangular");

     auto minimalconditionE = env->addEnumType("minimalCondition", "Minimal Condition");
     minimalconditionE->addValue("ACM-Loop");
     minimalconditionE->addValue("Constant");

     auto errorinsertionE = env->addEnumType("errorInsertion", "Error Insertion");
     errorinsertionE->addValue("Gate");

     auto delayE = env->addEnumType("delayType", "Delay Type");
     delayE->addValue("ConstantDelay");
     delayE->addValue("FileDelay");

     auto debugvalueE = env->addEnumType("debugValue", "Debug Value");
     debugvalueE->addValue("Debug");
     debugvalueE->addValue("Info");
     debugvalueE->addValue("Notice");
     debugvalueE->addValue("Error");
     debugvalueE->addValue("Critical");
     debugvalueE->addValue("Warning");

     auto globalC = mm->addComponent("global", "Global", "Some global parameters");
     auto lanASC = globalC->addList("lan_adaptation_schemes", "LAN Adaptation Schemes", "LAN adaptation, header compression/suppression schemes");
     lanASC->getPattern()->addParameter(BYTE, "pos", "Position")->setDefaultValue<Byte>(0);
     lanASC->getPattern()->addParameter("lanproto", "proto", "Protocol")->setDefaultValue<String>("IP");

     auto dvbnccC = mm->addComponent("dvb_ncc", "DVB NCC", "The DVB layer configuration for NCC. For Layer 2 FIFO configuration: check the Lan Adaptation plugins configuration below in order to get correct QoS mapping ; access type has to be correlated with the band configuration one");
     dvbnccC->addParameter("damaAgentAlgorithm", "dama_algorithm", "DAMA Algorithm", "DAMA Algorithm for controller")->setDefaultValue<String>("Legacy");
     dvbnccC->addParameter(INT, "fca", "Free Capacity Assignement", "The Free capacity assignement", "Kbps")->setDefaultValue<Int>(0);

     auto spotsL = dvbnccC->addList("spots", "Spots List");
     spotsL->getPattern()->addParameter(INT, "id", "ID")->setDefaultValue<Int>(1);
     auto simulationP = spotsL->getPattern()->addParameter("simuType", "simulation", "Simulation", "Activate simulation requests");
     simulationP->setDefaultValue<String>("None");
     auto simu_fileP = spotsL->getPattern()->addParameter(STRING, "simu_file", "Simulation File", "If simulation = file: use a file name or stdin");
     simu_fileP->setDefaultValue<String>("/etc/opensand/simulation/dama_spot1.input");
     simu_fileP->setReference(simulationP, "File");
     auto simu_random_nb_stationP = spotsL->getPattern()->addParameter(INT, "nb_station", "Station Number", "Numbered > 31");
     simu_random_nb_stationP->setDefaultValue<Int>(10);
     simu_random_nb_stationP->setReference(simulationP, "Random");
     auto simu_random_rt_bwP = spotsL->getPattern()->addParameter(INT, "rt_bandwidth", "RT Bandwidth", "", "Kbps");
     simu_random_rt_bwP->setDefaultValue<Int>(100);
     simu_random_rt_bwP->setReference(simulationP, "Random");
     auto simu_random_max_rbdcP = spotsL->getPattern()->addParameter(INT, "max_rbdc", "Maximum RBDC", "", "Kbps");
     simu_random_max_rbdcP->setDefaultValue<Int>(1024);
     simu_random_max_rbdcP->setReference(simulationP, "Random");
     auto simu_random_max_vbdcP = spotsL->getPattern()->addParameter(INT, "max_vbdc", "Maximum VBDC", "", "Kbps");
     simu_random_max_vbdcP->setDefaultValue<Int>(55);
     simu_random_max_vbdcP->setReference(simulationP, "Random");
     auto simu_random_mean_requestsP = spotsL->getPattern()->addParameter(INT, "mean_requests", "Mean Requests", "", "Kbps");
     simu_random_mean_requestsP->setDefaultValue<Int>(200);
     simu_random_mean_requestsP->setReference(simulationP, "Random");
     auto simu_random_amplitude_requestsP = spotsL->getPattern()->addParameter(INT, "amplitude_requests", "Amplitude Requests", "", "Kbps");
     simu_random_amplitude_requestsP->setDefaultValue<Int>(100);
     simu_random_amplitude_requestsP->setReference(simulationP, "Random");
     spotsL->getPattern()->addParameter(STRING, "event_file", "Event File", "Do we generate an event history ? (can be used for replaying a case study) (none, stdout, stderr, {file path})");
     auto l2fifosL = spotsL->getPattern()->addList("layer2_fifos", "Layer 2 FIFOs", "The MAC FIFOs");
     l2fifosL->getPattern()->addParameter(INT, "priority", "Priority", "The scheduler priority of the class related to the FIFO")->setDefaultValue<Int>(0);
     l2fifosL->getPattern()->addParameter("className", "name", "Name", "The name of the FIFO")->setDefaultValue<String>("NM");
     l2fifosL->getPattern()->addParameter(INT, "size_max", "Maximum Size", "The maximum number of cells or packets in DVB FIFO", "Packets")->setDefaultValue<Int>(1000);
     l2fifosL->getPattern()->addParameter("accessType", "access_type", "Access Type", "The type of capacity access for the scheduler")->setDefaultValue<String>("ACM");

     auto spots2L = dvbnccC->addList("spots2", "Spots List");
     spots2L->getPattern()->addParameter(INT, "id", "ID")->setDefaultValue<Int>(1);
     simulationP = spots2L->getPattern()->addParameter("simuType", "simulation", "Simulation", "Activate simulation requests");
     simulationP->setDefaultValue<String>("None");
     simu_fileP = spots2L->getPattern()->addParameter(STRING, "simu_file", "Simulation File", "If simulation = file: use a file name or stdin");
     simu_fileP->setDefaultValue<String>("/etc/opensand/simulation/dama_spot1.input");
     simu_fileP->setReference(simulationP, "File");
     simu_random_nb_stationP = spots2L->getPattern()->addParameter(INT, "nb_station", "Station Number", "Numbered > 31");
     simu_random_nb_stationP->setDefaultValue<Int>(10);
     simu_random_nb_stationP->setReference(simulationP, "Random");
     simu_random_rt_bwP = spots2L->getPattern()->addParameter(INT, "rt_bandwidth", "RT Bandwidth", "", "Kbps");
     simu_random_rt_bwP->setDefaultValue<Int>(100);
     simu_random_rt_bwP->setReference(simulationP, "Random");
     simu_random_max_rbdcP = spots2L->getPattern()->addParameter(INT, "max_rbdc", "Maximum RBDC", "", "Kbps");
     simu_random_max_rbdcP->setDefaultValue<Int>(1024);
     simu_random_max_rbdcP->setReference(simulationP, "Random");
     simu_random_max_vbdcP = spots2L->getPattern()->addParameter(INT, "max_vbdc", "Maximum VBDC", "", "Kbps");
     simu_random_max_vbdcP->setDefaultValue<Int>(55);
     simu_random_max_vbdcP->setReference(simulationP, "Random");
     simu_random_mean_requestsP = spots2L->getPattern()->addParameter(INT, "mean_requests", "Mean Requests", "", "Kbps");
     simu_random_mean_requestsP->setDefaultValue<Int>(200);
     simu_random_mean_requestsP->setReference(simulationP, "Random");
     simu_random_amplitude_requestsP = spots2L->getPattern()->addParameter(INT, "amplitude_requests", "Amplitude Requests", "", "Kbps");
     simu_random_amplitude_requestsP->setDefaultValue<Int>(100);
     simu_random_amplitude_requestsP->setReference(simulationP, "Random");
     spots2L->getPattern()->addParameter(STRING, "event_file", "Event File", "Do we generate an event history ? (can be used for replaying a case study) (none, stdout, stderr, {file path})");
     l2fifosL = spots2L->getPattern()->addList("layer2_fifos", "Layer 2 FIFOs", "The MAC FIFOs");
     l2fifosL->getPattern()->addParameter(INT, "priority", "Priority", "The scheduler priority of the class related to the FIFO")->setDefaultValue<Int>(0);
     l2fifosL->getPattern()->addParameter("className", "name", "Name", "The name of the FIFO")->setDefaultValue<String>("NM");
     l2fifosL->getPattern()->addParameter(INT, "size_max", "Maximum Size", "The maximum number of cells or packets in DVB FIFO", "Packets")->setDefaultValue<Int>(1000);
     l2fifosL->getPattern()->addParameter("accessType", "access_type", "Access Type", "The type of capacity access for the scheduler")->setDefaultValue<String>("ACM");

     auto slottedAlohaC = mm->addComponent("slotted_aloha", "Slotted ALOHA", "The Slotted Aloha GW parameters");
     auto spotsAlohaL = slottedAlohaC->addList("spots", "Spots List");
     spotsAlohaL->getPattern()->addParameter(INT, "id", "ID")->setDefaultValue<Int>(1);
     spotsAlohaL->getPattern()->addParameter("sa_algo", "algorithm", "Algorithm", "The algorithm used to handle collisions on slots")->setDefaultValue<String>("CRDSA");
     auto stL = spotsAlohaL->getPattern()->addList("simulation_traffic", "Simulation Traffic", "Add Slotted Aloha simulated traffic in categories");
     stL->getPattern()->addParameter("category", "category", "Category", "The name of the category to which the traffic applies")->setDefaultValue<String>("Standard");
     stL->getPattern()->addParameter(INT, "nb_max_packets", "Maximum number of packets", "he maximum number of packets per Slotted Aloha frame per simulated terminal (0 to disable this line)")->setDefaultValue<Int>(0);
     stL->getPattern()->addParameter(INT, "nb_replicas", "Replicas Number", "The number of replicas per Slotted Aloha frame (including the original packet)")->setDefaultValue<Int>(2);
     stL->getPattern()->addParameter(INT, "ratio", "Ratio", "The amount of traffic to simulate on the category", "%")->setDefaultValue<Int>(20);

     auto qospepC = mm->addComponent("qospep", "QoS PEP", "The QoS PEP (Policy Enforcement Point) parameters");
     qospepC->addParameter(INT, "pep_to_dama_port", "PEP to DAMA port", "Communication port on DAMA for QoS PEP messages")->setDefaultValue<Int>(5333);
     qospepC->addParameter(INT, "pep_alloc_delay", "PEP Allocation Delay", "Delay to apply anticipation RBDC allocations from QoS PEP/ARC", "ms");

     auto svnointerfaceC = mm->addComponent("svno_interface", "SVNO Interface", "The SVNO interface parameters");
     svnointerfaceC->addParameter(INT, "svno_to_ncc_port", "SVNO to NCC port", "Communication port on NCC for SVNO messages")->setDefaultValue<Int>(5334);

     auto uplinkPLC = mm->addComponent("uplink_physical_layer", "Uplink Physical Layer", "The physical layer parameters, for uplink");
     uplinkPLC->addParameter("attenuationType", "attenuation_model_type", "Attenuation Model Type", "The type of attenuation model")->setDefaultValue<String>("Ideal");
     uplinkPLC->addParameter(INT, "clear_sky_condition", "Clear Sky Condition", "The clear sky C/N", "dB")->setDefaultValue<Int>(20);

     auto downlinkPLC = mm->addComponent("downlink_physical_layer", "Downlink Physical Layer", "The physical layer parameters, for downlink");
     downlinkPLC->addParameter("attenuationType", "attenuation_model_type", "Attenuation Model Type", "The type of attenuation model")->setDefaultValue<String>("Ideal");
     downlinkPLC->addParameter("minimalCondition", "minimal_condition_type", "Minimal Condition Type", "The type of minimal conditions")->setDefaultValue<String>("ACM-Loop");
     downlinkPLC->addParameter("errorInsertion", "error_insertion_type", "Error Insertion Type", "The type of error insertion")->setDefaultValue<String>("Gate");
     downlinkPLC->addParameter(INT, "clear_sky_condition", "Clear Sky Condition", "The clear sky C/N", "dB")->setDefaultValue<Int>(20);

     auto delayC = mm->addComponent("delay", "Delay", "Satellite delay configuration");
     delayC->addParameter("delayType", "delay_type", "Delay Type", "The type of delay associated to the terminal")->setDefaultValue<String>("ConstantDelay");
     delayC->addParameter(INT, "refresh_period", "Refresh Period", "Satellite delay refresh period", "ms")->setDefaultValue<Int>(1000);

     auto interconnectC = mm->addComponent("interconnect", "Interconnect", "Split-GW interconnect configuration");
     interconnectC->addParameter(INT, "upward_data_port", "Upward Data Port", "The UDP port used for upward data communications")->setDefaultValue<Int>(54996);
     interconnectC->addParameter(INT, "upward_sig_port", "Upward SIG Port", "The UDP port used for upward signalling communications")->setDefaultValue<Int>(54997);
     interconnectC->addParameter(INT, "downward_data_port", "Downward Data Port", "The UDP port used for downward data communications")->setDefaultValue<Int>(54998);
     interconnectC->addParameter(INT, "downward_sig_port", "Downward SIG Port", "The UDP port used for downward signalling communications")->setDefaultValue<Int>(54999);
     interconnectC->addParameter(STRING, "upper_ip_address", "Upper IP Address", "IP address of the upper Interconnect block")->setDefaultValue<String>("192.168.17.2");
     interconnectC->addParameter(STRING, "lower_ip_address", "Lower IP Address", "IP address of the lower Interconnect block")->setDefaultValue<String>("192.168.17.1");
     interconnectC->addParameter(INT, "interconnect_udp_rmem", "Interconnect UDP RMEM", "The size of the UDP reception buffer in kernel for interconnect sockets")->setDefaultValue<Int>(1048580);
     interconnectC->addParameter(INT, "interconnect_udp_wmem", "Interconnect UDP WMEM", "The size of the UDP emission buffer in kernel for interconnect sockets")->setDefaultValue<Int>(1048580);
     interconnectC->addParameter(SHORT, "interconnect_udp_stack", "Interconnect UDP Stack", "The size of the UDP stack in interconnect sockets")->setDefaultValue<Short>(5);

     auto debugC = mm->addComponent("debug", "Debug", "For levels table, you can choose any available logs or part of log name. After a first simulation, autocompletion will be availabble for level names.");
     debugC->addParameter("debugValue", "init", "Initialization")->setDefaultValue<String>("Warning");
     debugC->addParameter("debugValue", "lan_adaptation", "LAN Adaptation")->setDefaultValue<String>("Warning");
     debugC->addParameter("debugValue", "encap", "Encapsulation")->setDefaultValue<String>("Warning");
     debugC->addParameter("debugValue", "dvb", "DVB")->setDefaultValue<String>("Warning");
     debugC->addParameter("debugValue", "physical_layer", "Physical Layer")->setDefaultValue<String>("Warning");
     debugC->addParameter("debugValue", "sat_carrier", "SAT Carrier")->setDefaultValue<String>("Warning");
     auto levelsL = debugC->addList("levels", "Levels", "The user log levels");
     levelsL->getPattern()->addParameter(STRING, "name", "Name", "A log name or part of the name")->setDefaultValue<String>("default");
     levelsL->getPattern()->addParameter("debugValue", "level", "Level", "The debug level")->setDefaultValue<String>("Warning");
}

void buildOpenSANDModelSimplified(Model *mm, Environment *env)
{
     auto lanprotoE = env->addEnumType("lanproto", "LAN Protocol");
     lanprotoE->addValue("IP");
     lanprotoE->addValue("Ethernet");
     lanprotoE->addValue("ROHC");
     lanprotoE->addValue("PHS");

     auto damaAgentAlgorithmE = env->addEnumType("damaAgentAlgorithm", "DAMA Agent Algorithm");
     damaAgentAlgorithmE->addValue("Legacy");
     damaAgentAlgorithmE->addValue("RrmQos");

     auto simutypeE = env->addEnumType("simuType", "Simulation Type");
     simutypeE->addValue("None");
     simutypeE->addValue("File");
     simutypeE->addValue("Random");

     auto classnameE = env->addEnumType("className", "Class Name");
     classnameE->addValue("NM");
     classnameE->addValue("EF");
     classnameE->addValue("SIG");
     classnameE->addValue("AF");
     classnameE->addValue("BE");

     auto accesstypeE = env->addEnumType("accessType", "Access Type");
     accesstypeE->addValue("ACM");
     accesstypeE->addValue("VCM0");
     accesstypeE->addValue("VCM1");
     accesstypeE->addValue("VCM2");
     accesstypeE->addValue("VCM3");

     auto saalgoE = env->addEnumType("sa_algo", "SA Algorithm");
     saalgoE->addValue("DSA");
     saalgoE->addValue("CRDSA");

     auto categoryE = env->addEnumType("category", "Category");
     categoryE->addValue("Standard");
     categoryE->addValue("Premium");
     categoryE->addValue("Pro");
     categoryE->addValue("SVNO1");
     categoryE->addValue("SVNO2");
     categoryE->addValue("SVNO3");
     categoryE->addValue("SNO");

     auto attenuationtypeE = env->addEnumType("attenuationType", "Attenuation Type");
     attenuationtypeE->addValue("Ideal");
     attenuationtypeE->addValue("File");
     attenuationtypeE->addValue("On/Off");
     attenuationtypeE->addValue("Triangular");

     auto minimalconditionE = env->addEnumType("minimalCondition", "Minimal Condition");
     minimalconditionE->addValue("ACM-Loop");
     minimalconditionE->addValue("Constant");

     auto errorinsertionE = env->addEnumType("errorInsertion", "Error Insertion");
     errorinsertionE->addValue("Gate");

     auto delayE = env->addEnumType("delayType", "Delay Type");
     delayE->addValue("ConstantDelay");
     delayE->addValue("FileDelay");

     auto debugvalueE = env->addEnumType("debugValue", "Debug Value");
     debugvalueE->addValue("Debug");
     debugvalueE->addValue("Info");
     debugvalueE->addValue("Notice");
     debugvalueE->addValue("Error");
     debugvalueE->addValue("Critical");
     debugvalueE->addValue("Warning");

     auto globalC = mm->addComponent("global", "Global", "Some global parameters");
     auto lanASC = globalC->addList("lan_adaptation_schemes", "LAN Adaptation Schemes", "LAN adaptation, header compression/suppression schemes");
     lanASC->getPattern()->addParameter(BYTE, "pos", "Position")->setDefaultValue<Byte>(0);
     lanASC->getPattern()->addParameter("lanproto", "proto", "Protocol")->setDefaultValue<String>("IP");

     auto dvbnccC = mm->addComponent("dvb_ncc", "DVB NCC", "The DVB layer configuration for NCC. For Layer 2 FIFO configuration: check the Lan Adaptation plugins configuration below in order to get correct QoS mapping ; access type has to be correlated with the band configuration one");
     dvbnccC->addParameter("damaAgentAlgorithm", "dama_algorithm", "DAMA Algorithm", "DAMA Algorithm for controller")->setDefaultValue<String>("Legacy");
     dvbnccC->addParameter(INT, "fca", "Free Capacity Assignement", "The Free capacity assignement", "Kbps")->setDefaultValue<Int>(0);

     auto spotsL = dvbnccC->addList("spots", "Spots List");
     spotsL->getPattern()->addParameter(INT, "id", "ID")->setDefaultValue<Int>(1);
     auto simulationP = spotsL->getPattern()->addParameter("simuType", "simulation", "Simulation", "Activate simulation requests");
     simulationP->setDefaultValue<String>("None");
     auto simu_fileP = spotsL->getPattern()->addParameter(STRING, "simu_file", "Simulation File", "If simulation = file: use a file name or stdin");
     simu_fileP->setDefaultValue<String>("/etc/opensand/simulation/dama_spot1.input");
     simu_fileP->setReference(simulationP, "File");
     auto simu_random_nb_stationP = spotsL->getPattern()->addParameter(INT, "nb_station", "Station Number", "Numbered > 31");
     simu_random_nb_stationP->setDefaultValue<Int>(10);
     simu_random_nb_stationP->setReference(simulationP, "Random");
     auto simu_random_rt_bwP = spotsL->getPattern()->addParameter(INT, "rt_bandwidth", "RT Bandwidth", "", "Kbps");
     simu_random_rt_bwP->setDefaultValue<Int>(100);
     simu_random_rt_bwP->setReference(simulationP, "Random");
     auto simu_random_max_rbdcP = spotsL->getPattern()->addParameter(INT, "max_rbdc", "Maximum RBDC", "", "Kbps");
     simu_random_max_rbdcP->setDefaultValue<Int>(1024);
     simu_random_max_rbdcP->setReference(simulationP, "Random");
     auto simu_random_max_vbdcP = spotsL->getPattern()->addParameter(INT, "max_vbdc", "Maximum VBDC", "", "Kbps");
     simu_random_max_vbdcP->setDefaultValue<Int>(55);
     simu_random_max_vbdcP->setReference(simulationP, "Random");
     auto simu_random_mean_requestsP = spotsL->getPattern()->addParameter(INT, "mean_requests", "Mean Requests", "", "Kbps");
     simu_random_mean_requestsP->setDefaultValue<Int>(200);
     simu_random_mean_requestsP->setReference(simulationP, "Random");
     auto simu_random_amplitude_requestsP = spotsL->getPattern()->addParameter(INT, "amplitude_requests", "Amplitude Requests", "", "Kbps");
     simu_random_amplitude_requestsP->setDefaultValue<Int>(100);
     simu_random_amplitude_requestsP->setReference(simulationP, "Random");
     spotsL->getPattern()->addParameter(STRING, "event_file", "Event File", "Do we generate an event history ? (can be used for replaying a case study) (none, stdout, stderr, {file path})");
     auto l2fifosL = spotsL->getPattern()->addList("layer2_fifos", "Layer 2 FIFOs", "The MAC FIFOs");
     l2fifosL->getPattern()->addParameter(INT, "priority", "Priority", "The scheduler priority of the class related to the FIFO")->setDefaultValue<Int>(0);
     l2fifosL->getPattern()->addParameter("className", "name", "Name", "The name of the FIFO")->setDefaultValue<String>("NM");
     l2fifosL->getPattern()->addParameter(INT, "size_max", "Maximum Size", "The maximum number of cells or packets in DVB FIFO", "Packets")->setDefaultValue<Int>(1000);
     l2fifosL->getPattern()->addParameter("accessType", "access_type", "Access Type", "The type of capacity access for the scheduler")->setDefaultValue<String>("ACM");

     auto slottedAlohaC = mm->addComponent("slotted_aloha", "Slotted ALOHA", "The Slotted Aloha GW parameters");
     auto spotsAlohaL = slottedAlohaC->addList("spots", "Spots List");
     spotsAlohaL->getPattern()->addParameter(INT, "id", "ID")->setDefaultValue<Int>(1);
     spotsAlohaL->getPattern()->addParameter("sa_algo", "algorithm", "Algorithm", "The algorithm used to handle collisions on slots")->setDefaultValue<String>("CRDSA");
     auto stL = spotsAlohaL->getPattern()->addList("simulation_traffic", "Simulation Traffic", "Add Slotted Aloha simulated traffic in categories");
     stL->getPattern()->addParameter("category", "category", "Category", "The name of the category to which the traffic applies")->setDefaultValue<String>("Standard");
     stL->getPattern()->addParameter(INT, "nb_max_packets", "Maximum number of packets", "he maximum number of packets per Slotted Aloha frame per simulated terminal (0 to disable this line)")->setDefaultValue<Int>(0);
     stL->getPattern()->addParameter(INT, "nb_replicas", "Replicas Number", "The number of replicas per Slotted Aloha frame (including the original packet)")->setDefaultValue<Int>(2);
     stL->getPattern()->addParameter(INT, "ratio", "Ratio", "The amount of traffic to simulate on the category", "%")->setDefaultValue<Int>(20);

     auto uplinkPLC = mm->addComponent("uplink_physical_layer", "Uplink Physical Layer", "The physical layer parameters, for uplink");
     uplinkPLC->addParameter("attenuationType", "attenuation_model_type", "Attenuation Model Type", "The type of attenuation model")->setDefaultValue<String>("Ideal");
     uplinkPLC->addParameter(INT, "clear_sky_condition", "Clear Sky Condition", "The clear sky C/N", "dB")->setDefaultValue<Int>(20);

     auto downlinkPLC = mm->addComponent("downlink_physical_layer", "Downlink Physical Layer", "The physical layer parameters, for downlink");
     downlinkPLC->addParameter("attenuationType", "attenuation_model_type", "Attenuation Model Type", "The type of attenuation model")->setDefaultValue<String>("Ideal");
     downlinkPLC->addParameter("minimalCondition", "minimal_condition_type", "Minimal Condition Type", "The type of minimal conditions")->setDefaultValue<String>("ACM-Loop");
     downlinkPLC->addParameter("errorInsertion", "error_insertion_type", "Error Insertion Type", "The type of error insertion")->setDefaultValue<String>("Gate");
     downlinkPLC->addParameter(INT, "clear_sky_condition", "Clear Sky Condition", "The clear sky C/N", "dB")->setDefaultValue<Int>(20);
}
