Vue.component("monitoring_panel", {
  delimiters: ["[[", "]]"],
  props: { "on" : Boolean  },
  data: function () {
    return {
      c_polling: null,
    };
  },
  mounted() {},
  beforeDestroy() {},
  methods: {
    getLatestData() {

    },
  },
  template: `
    <div>
      <table border="1" bordercolor="#000000" cellspacing="0" cellpadding="0">
        <tbody>
          <tr>
            <td style="font-weight: bold;">Run</td>
            <td>number:</td>
            <td>Starting time:</td>
          </tr>
          <tr>
            <td style="font-weight: bold;">Physics</td>
            <td>events</td>
            <td>Hz</td>
          </tr>
          <tr>
            <td style="font-weight: bold;">Monitoring</td>
            <td>events</td>
            <td>Hz</td>
          </tr>
          <tr>
            <td style="font-weight: bold;">Calibration</td>
            <td>events</td>
            <td>Hz</td>
          </tr>
        </tbody>
      </table>
    </div>
  `,
});

var app = new Vue({
  el: "#app",
  vuetify: new Vuetify(),
  data: {
    drawer: false, // if the tree view is closed or not
    isDrawerLocked: false, // if the tree view in is in locked state
    c_configDirs: [],
    c_treeJson: {},
    c_loadedConfigName: null, // the name of the config file
    c_configLoading: false,
    c_socket: io(), // websockets
    activeNode: [],
    nodeStates: null,
    stateColors: {
      // for state color
      not_added: "grey",
      added: "brown",
      booted: "blue",
      ready: "yellow",
      running: "green",
      paused: "orange",
      error: "red",
      READY: "yellow",
      RUN : "green",
      DOWN : "grey lighten-1"
    },
    log: [],
    fsmRules:null,
    processing: {
      // if button is in processing state (MAJ for ROOT commands)
      add: false,
      boot: false,
      configure: false,
      start: false,
      stop: false,
      unconfigure: false,
      shutdown: false,
      remove: false,
      INITIALISE: false,
      START: false,
      STOP: false,
      SHUTDOWN: false,
      PAUSE: false,
      ECR: false,
    },
    logged: false, // if the user is logged using cern account
    infoIsActive: false, // if the info box is active

    runState: "",
    runOnGoing: false
  },
  delimiters: ["[[", "]]"],
  mounted() {
    this.getInitRunState() 
    this.getConfigDirs();
    this.getLog();
    // listeners
    this.c_socket.on("logChng", (line) => {
      this.log.push(line);
    });

    this.c_socket.on("runStateChng", (info) => {
      this.runState = info["runState"]
      if ((info["runState"] != "DOWN") && info["file"] != this.c_loadedConfigName ){
        console.log("Switching configuration file")
        this.loadConfig(info["file"])
      }
    })
  },
  methods: {
    isROOTButtonEnabled(RCommand){
      // console.log(this.fsmRules ? this.fsmRules[this.runState].includes(RCommand): "Pas chargé")
      // TODO: add condition if logged in or not 
      return this.fsmRules ? this.fsmRules[this.runState].includes(RCommand): false;
    },
    goToRunningConfig(runningConfig){
      console.log("Going to ", runningConfig )
    },
    getInitRunState(){
      axios.get("/runState").then((response)=> {
        data = response["data"]
        this.runState = data["runState"]
        this.runOnGoing = data["runOngoing"]
        if (this.runOnGoing) {
          this.loadConfig(data["runningFile"]);
        }
      })
    },
    getFSM() {
      axios.get("fsmrulesJson").then((response) => {
        if (response.data.length == 0) {
        }
        this.fsmRules = response.data;
        this.fsmRules["default"] = [];
      });
    },
    executeCommand(command) {
      if (this.activeNode.length != 0) {
        this.processing[command] = true;
        axios
          .post("/ajaxParse", {
            node: this.activeNode[0].name,
            command: command,
          })
          .then(() => {
            console.log("executeCommand: Recieved response");
            this.processing[command] = false;
          })
          .catch((e) => {
            // if there is a problem when sending the command
            this.processing[command] = false;
          });
      } else {
        alert("Choose first a node");
      }
    },
    getLog() {
      axios.get("log").then((response) => {
        this.log = response.data;
      });
    },
    setActiveNode(node) {
      this.activeNode = node;
    },
    getConfigDirs() {
      axios.get("/configDirs").then((response) => {
        this.c_configDirs = response.data;
      });
    },
    loadConfig(config) {
      this.c_configLoading = true;
      this.infoIsActive = false;
      this.nodeStates = null;
      this.c_treeJson = null;
      this.activeNode = [];
      axios
        .get("initConfig", { params: { configName: config } })
        .then((_) => {
          axios.get("/urlTreeJson").then((response) => {
            this.c_treeJson = response.data;

            axios.get("statesList").then((response) => {
              this.nodeStates = response.data;
              this.c_loadedConfigName = config;
              this.getFSM();
              this.c_configLoading = false;
            });
          });
        })
        .catch((error) => {
          this.c_configLoading = false;
          console.error("ERROR: ", error);
        });
    },
    getNodeState(name) {
      // name is a string
      if (name && this.nodeStates) {
        return Array.isArray(this.nodeStates[name][0])
          ? this.nodeStates[name][0][0]
          : this.nodeStates[name][0];
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
    login() {
      window.location.replace("/login");
    },
    logout() {
      // axios.get("/logout");
      window.location.replace("/logout");
    },
    hasChildren(node) {
      // if log info btns are displayed
      if (node) {
        return "children" in node[0] ? true : false;
      } else {
        return true;
      }
    },
    sendROOTCommand(action) {
      this.processing[action] = true;
      axios
        .get(`/processROOTCommand?command=${action}`)
        .then(() => {
          console.log(action);
          this.processing[action] = false;
          console.log("Process completed");
        })
        .catch((e) => {
          this.processing[action] = false;
          console.log("ERROR:", e);
        });
    },
    isActiveNode() {
      return this.activeNode.length != 0;
    },
    openInfoInNewWindow() {
      this.infoIsActive = false;
      window.open(
        `/infoWindow/${this.activeNode[0].name}`,
        "_blank",
        "width=800, height=500"
      );
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
  computed: {
    rootNode: function () {
      // can have a different name than root
      return this.c_treeJson["name"];
    },
    // runOnGoing: function(){
    //   return false ? this.runState == "DOWN" : true;
    // }
  },
});
