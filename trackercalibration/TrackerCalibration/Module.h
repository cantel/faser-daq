#ifndef __Module_h__
#define __Module_h__

#include <vector>
#include <string>

#include "nlohmann/json.hpp"

namespace TrackerCalib {

  /// forward includes from within namespace TrackerCalib
  class Chip;

  /** 
   * @class Module
   *    
   * @brief describing SCT module for calibration purposes
   *
   */
  class Module {    

  public:
    /** \brief Default constructor */
    Module();
    
    /** \brief Constructor from json configuration file*/
    Module(std::string jsconCfg, int printLevel=0);
    
    /**  \brief Copy constructor */
    Module(const Module&);
    
    /**  \brief operator = (typically used in copy constructor) */
    Module &operator=(const Module&);
    
    /** \brief  Destructor */
    ~Module();

   /** \brief Populate module from json configuration file.
     *
     * This function will parse the json file and create the 
     * vector of chips of the module. Also the module ID, trbChannel 
     * and planeID data members will be set.
     */
    int readJson(std::string jsonCfg);

    /** write output json configuration file with current module data. */
    int writeJson(std::string outDir);

    /** Set verbosity level */
    void setPrintLevel(int printLevel);

    /** Get module ID */
    unsigned long id() const { return m_id; }

    /** Get planeID */
    unsigned int planeId() const { return m_planeID; }

    /** Get TRB channel */
    int trbChannel() const { return m_trbChannel; }

    /** Get module mask */
    uint8_t moduleMask() const { return m_moduleMask; }

    /** return INITIAL json configuration file */
    std::string cfgFile() const { return m_jsonCfg; }

    /** return LAST json configuration file */
    std::string cfgFileLast() const { return m_jsonCfgLast; }

    /** Return the total number of active channels in module (to be used in NoiseOccupancy). */
    int nActiveChannels();
    
    /** \brief find chip from address 
     *
     * Function that returns a pointer to a Chip object if
     * found according to the address argument.
     */
    Chip *findChip(const unsigned int);

    /** Returns vector of chips. */
    std::vector<Chip*>& Chips() { return m_chips; }

    /**
     * Show attributes of module.
     * @param[in] indent Indent level of printout
     */
    const std::string print(int indent=0);


  private:
    /** add new Chip to module */
    int addChip(Chip *chip);    
    
  private:
    unsigned long m_id; /// moduleID (i.e. module serial number)
    unsigned int m_planeID; /// planeID the module belongs to
    unsigned int m_trbChannel; /// TRB channel the module is connected to    
    uint8_t m_moduleMask; /// module mask
    std::vector<Chip*> m_chips; /// Vector of chips
    std::string m_jsonCfg;     /// Initial configuration file for this module.
    std::string m_jsonCfgLast; /// Final configuration file for this module

    int m_printLevel; /// Verbosity level
    
  };  // class Module

} // namespace TrackerCalib

#endif 
