var app = new Vue({
  el: "#app",
  vuetify: new Vuetify(),
  data: {
    drawer: true,
    isDrawerLocked: false,
    c_configDirs: [],
    c_treeJson: {},
    c_loadedConfigName: null,
    c_configLoading: false,
    c_socket: io(),
    activeNode: [],
    nodeStates: {},
    stateColors: {
      not_added: "grey",
      added: "brown",
      booted: "blue",
      ready: "yellow",
      running: "green",
      paused: "orange",
      error: "red",
    },
    log: [],
    fsmRules : {"default":[]},
    processing : {
      "add": false,
      "boot": false,
      "configure":false,
      "start" : false,
      "stop" : false,
      "unconfigure" :  false,
      "shutdown": false,
      "remove": false
    }
    
  },
  delimiters: ["[[", "]]"],
  mounted() {
    this.getConfigDirs();
    this.getLog();
    this.c_socket.on("logChng", (line) => {
      this.log.push(line);
    });
  },
  methods: {
    getFSM(){
      axios.get("fsmrulesJson").then((response)=>{
        this.fsmRules = response.data
        this.fsmRules["default"] = []
        
      })
    },
    executeCommand(command) {
      if (this.activeNode.length != 0){
        this.processing[command] = true;
      axios
        .post("/ajaxParse", {
          node: this.activeNode[0] ,
          command: command,
        })
        .then((r) => {
          console.log("executeCommand: Recieved response");
          this.processing[command] = false;
        });
      }
      else {
        alert("Choissez d'abord un node")
      }
    },
    getLog() {
      axios.get("log").then((response) => {
        this.log = response.data;
      });
    },
    getActiveNodeInfo(node) {
      this.activeNode = node;
    },
    getConfigDirs() {
      axios.get("/configDirs").then((response) => {
        this.c_configDirs = response.data;
      });
    },
    loadConfig(config) {
      this.c_configLoading = true;
      

      axios.get("initConfig", { params: { configName: config } }).then((_) => {
        console.log("config loaded");
        axios.get("/urlTreeJson").then((response) => {
          this.c_treeJson = response.data;

          axios.get("statesList").then((response) => {
            this.nodeStates = response.data;
            this.c_loadedConfigName = config;
            this.getFSM();
            this.c_configLoading = false;
          });
        });
      });
    },
    getNodeState(node) {
      // nodeStates[node] can be a array with one element or a string
      if (node) {
        return Array.isArray(this.nodeStates[node][0])
          ? this.nodeStates[node][0][0]
          : this.nodeStates[node][0];
      } else {
        return "default";
      }
    },
    processIncludeExclude(node, status) {
      // if current state is "include" -> set to exclude
      if (status) {
        console.log(node, "will be excluded");
        axios
          .post("/ajaxParse", {
            node: node,
            command: "exclude",
          })
          .then((r) => {
            console.info(node, "changed");
          });
      } else {
        console.log(node, "will be included");
        axios
          .post("/ajaxParse", {
            node: node,
            command: "include",
          })
          .then((r) => {
            console.info(node, "changed");
          });
      }
    }, 
  },
  watch: {
    c_loadedConfigName: {
      handler: function (newConfig, oldConfig) {
        console.log("old", oldConfig, "new", newConfig);
        // we close the old listener for nodeStates
        this.c_socket.off("stsChng" + oldConfig, function () {
          console.log("removed listener");
        });
        // we create the new listener for nodeStates
        this.c_socket.on("stsChng" + newConfig, (result) => {
          this.nodeStates = result;
        });
      },
    },
  },
});
