Vue.component("sequencer_dialog", {
  delimiters: ["[[", "]]"],
  props: {
    "sequencer_state": Object,
    "last_sequencer_state": Object,
  },
  data: function () {
    return {
      dialog: true,
      selected_config: "",
      seqNumber: 0,
      stepNumber: 2,
      loadingConfig: false,
      configFile: null,
      steps: [],
      listConfigs: [],
      expandedSteps: false,
      errorLoadedConfigMessage: "",
      stopping: false
    };
  },
  mounted() {
    if (Object.keys(this.sequencer_state).length == 0) {
      console.log("Getting the configs from the correct folder")
      axios.get("/getSequencerConfigs").then((response) => {
        this.listConfigs = response.data
      })
    }
    else {
      this.loadSequencerConfig(this.sequencer_state.sequenceName)
      this.expandedSteps = true;
    }
  },
  methods: {
    emit_stop() {
      this.$emit("stop_seq")
      this.stopping = true
    },
    loadSequencerConfig(selected_config) {
      axios.get("/loadSequencerConfig", { params: { "sequencerConfigName": selected_config } }).then((response) => {
        if (response.data.status == "error") {
          this.errorLoadedConfigMessage = response.data.data;
          console.log("Error, with the following message : ", response.data.data);
        }
        else {
          this.errorLoadedConfigMessage = "";
          this.steps = response.data.data.steps;
          this.stepNumber = 1 // preventing a bug from the slider in the sequencer rcgui dialog 
          this.configFile = response.data.data.config;
        }
        this.loadingConfig = false;
      })
    }
  },

  watch: {
    selected_config: {
      handler: function () {
        this.loadingConfig = true;
        this.loadSequencerConfig(this.selected_config);
      }
    }

  },
  template: /*html*/`
    <v-dialog v-model="dialog" width="500" persistent>
  <v-card>  
    <v-card-title>
          <span class="text-h5"> Sequencer : [[ Object.keys(this.sequencer_state).length == 0 ? '' : this.sequencer_state.sequenceName ]] </span>
    </v-card-title>
 

    <v-card-text>
      <v-row>
        <v-col>
          <span v-if ="Object.keys(last_sequencer_state).length != 0" class="text-caption">
            Last seq info : [[ this.last_sequencer_state.sequenceName]], 
            seqNumber : [[this.last_sequencer_state.seqNumber]], 
            step :  [[this.last_sequencer_state.stepNumber]] / [[this.last_sequencer_state.totalStepsNumber]]
          </span>
          </v-col>
      </v-row >

     <div v-if="Object.keys(sequencer_state).length == 0">
      <v-row>
        <v-col>
          <v-select v-model="selected_config" :items="listConfigs" label="Select sequence configuration file" outlined
            dense :hide-details="errorLoadedConfigMessage === ''" disable-lookup :readonly="loadingConfig" :loading="loadingConfig"
            :error="errorLoadedConfigMessage !== ''" :error-messages="errorLoadedConfigMessage"
            ></v-select>
        </v-col>
      </v-row>
      <template v-if="errorLoadedConfigMessage === ''">
      <v-row>
        <v-col cols='6'>
          <v-subheader>Seq number</v-subheader>
        </v-col>
        <v-spacer></v-spacer>
        <v-col cols="6">
          <v-text-field v-model="seqNumber" outlined dense hide-details></v-text-field>
        </v-col>
      </v-row>
      <v-row v-if="selected_config !== ''" justify="space-around">
        <v-col cols="12">
          <v-slider class="align-center" v-model="stepNumber" hide-details :min="1" :max="steps.length"
            label="Step number" thumb-label>
            <template v-slot:append>
              <v-text-field v-model="stepNumber" class="mt-0 pt-0" hide-details single-line type="number"
                style="width: 60px" :min="1" :max="steps.length"></v-text-field>
            </template>
          </v-slider>
        </v-col>
      </v-row>
      </template>
      </div> 
    </v-card-text>
    <v-expand-transition>
      <div v-if="expandedSteps">
        <v-card-text style="overflow:auto; white-space: nowrap;">
          <v-virtual-scroll :bunched="10" :items="steps" height="200" item-height="26">
            <template v-slot:default="{ item, index }">
              <div :key="index" :class=" index+1 === sequencer_state.stepNumber ? 'd-flex text-body-2 green lighten-2 py-2' : 'd-flex text-body-2 py-2' ">
                <span class="mx-2">[[index+1]] : </span>
                <span> [[ item.startcomment ]] </span>
              </div>
            </template>
          </v-virtual-scroll>
        </v-card-text>
      </div>
    </v-expand-transition>
    <v-card-actions>
        <v-btn color="error" @click="$emit('close_seq')">close</v-btn>
        <template v-if="Object.keys(sequencer_state).length == 0">
        <v-btn :disabled="selected_config==''" color="success"
          @click="$emit('start_seq', selected_config, seqNumber, stepNumber)">Start</v-btn>
      </template>
      <template v-else>
        <v-btn :disabled="stopping" color="danger" @click="emit_stop">[[ stopping ? "STOPPING..." :  "STOP"]]</v-btn>
      </template>
      <v-spacer></v-spacer>
      <v-btn text @click="expandedSteps = !expandedSteps" v-if="errorLoadedConfigMessage === ''">
        [[ expandedSteps ? 'hide steps' : 'show steps' ]]
      </v-btn>
    </v-card-actions>
  </v-card>
</v-dialog>
  `,
});

Vue.component("plot", {
  props: ["update", "name"],
  template: '<div :ref="name"></div>',
  data: function () {
    return {
      chart: {
        data: [
          {
            x: [],
            y: [],
            mode: "lines",
            line: { color: "#80CAF6" },
          },
        ],
        layout: {
          height: 200,
          margin: { l: 30, r: 10, b: 20, t: 0, pad: 0 },
          font: { size: 9 },
          xaxis: {
            showticklabels: false,
            autorange: true,
            rangeslider: {},
            type: "date",
          },
        },
        config: { responsive: true, displayModeBar: false, showTips: false },
      },
    };
  },
  mounted() {
    Plotly.newPlot(
      this.$refs[this.name],
      this.chart.data,
      this.chart.layout,
      this.chart.config
    );
  },
  watch: {
    update: {
      handler: function () {
        if (this.chart.layout.xaxis.showticklabels == false) {
          this.chart.layout.xaxis.showticklabels = true;
        }
        Plotly.extendTraces(this.$refs[this.name], this.update, [0]);
      },
      deep: true,
    },
  },
});

Vue.component("monitoring_panel", {
  delimiters: ["[[", "]]"],
  props: { p_on: Boolean },
  data: function () {
    return {
      c_polling: null,
      tab: 1,
      updates: {
        Physics: { x: [[]], y: [[]] },
        Calibration: { x: [[]], y: [[]] },
        TLBMonitoring: { x: [[]], y: [[]] },
      },
      metrics: {},
    };
  },
  mounted() {
    this.$watch(
      "p_on",
      (newV, oldV) => {
        if (oldV == true && newV == false) {
          // if we shutdown the run
          this.c_polling.close();
        } else if (newV && oldV == false) {
          axios.get("monitoring/initialValues").then((response) => {
            this.updateMetrics(response.data.values);
            this.populateGraphs(response.data.graphs);

            this.c_polling = new EventSource("monitoring/latestValues");
            this.c_polling.onmessage = (e) => {
              data = JSON.parse(e.data);
              this.updateMetrics(data.values);
              this.updateGraphs(data.graphs);
            };
          });
        }
      },
      { immediate: false }
    );
  },

  methods: {
    updateMetrics(metricsData) {
      this.metrics = metricsData;
    },
    populateGraphs(graphData) {
      let keys = ["Physics", "Calibration", "TLBMonitoring"];
      // update the graphs (calibration, monitoring, physics) in the monitoring panel
      keys.forEach((e) => {
        let x = Array.from(
          graphData[`History:eventbuilder01_Event_rate_${e}`][0],
          (x) => new Date(x * 1000)
        );
        let y = graphData[`History:eventbuilder01_Event_rate_${e}`][1];
        this.updates[e]["x"] = [x];
        this.updates[e]["y"] = [y];
      });
    },
    updateGraphs(graphData) {
      let keys = ["Physics", "Calibration", "TLBMonitoring"];
      keys.forEach((e) => {
        let x = new Date(graphData[`Event_rate_${e}`][0] * 1000);
        let y = graphData[`Event_rate_${e}`][1];
        this.updates[e]["x"] = [[x]]; // plotly wants Array[Array[]] format for the plots
        this.updates[e]["y"] = [[y]];
      });
    },
  },
  template: /*html*/`
    <div>
      <div class="d-flex flex-wrap justify-center mb-3 py-2">
        <v-card elevation="1" class="blue lighten-5 metrics-panel">
          <h4 class="text-center">Run</h4>
          <div class="text-body-2">
            <div>
              <strong class="blue--text text--darken-4">Number:</strong> [[metrics.RunNumber]] 
            </div>
            <div>
              <strong class="blue--text text--darken-4" >Starting time:</strong>  [[metrics.RunStart]]
            </div>
          </div>
        </v-card>
        <v-card elevation="1" class="blue lighten-5 metrics-panel ">
          <h4 class="text-center">Physics</h4>
          <div class="text-body-2">
            <div>
              <strong class="blue--text text--darken-4">Events:</strong> [[metrics.Events_received_Physics]] 
            </div>
            <div>
              <strong class="blue--text text--darken-4">Rate:</strong> [[metrics.Event_rate_Physics]] Hz
            </div>
          </div>
        </v-card>
        <v-card elevation="1" class=" blue lighten-5 metrics-panel">
          <h4 class="text-center">Calibration</h4>
          <div class="text-body-2">
            <div>
              <strong class="blue--text text--darken-4">Events:</strong> [[metrics.Events_received_Calibration]] 
            </div>
            <div>
              <strong class="blue--text text--darken-4">Rate:</strong> [[metrics.Event_rate_Calibration]] Hz
            </div>
          </div>
        </v-card>
        <v-card elevation="1" class="blue lighten-5 metrics-panel">
          <h4 class="text-center">Monitoring</h4>
          <div class="text-body-2">
            <div>
              <strong class="blue--text text--darken-4">Events:</strong> [[metrics.Events_received_TLBMonitoring]] 
            </div>
            <div>
              <strong class="blue--text text--darken-4">Rate:</strong> [[metrics.Event_rate_TLBMonitoring]] Hz
            </div>
          </div>
        </v-card>
      </div>
      <div>
        <v-tabs v-model="tab">
          <v-tabs-slider></v-tabs-slider>
          <v-tab href="#tab-1" class="text-capitalize"> Physics </v-tab>
          <v-tab href="#tab-2"class="text-capitalize"> Calibration </v-tab>
          <v-tab href="#tab-3" class="text-capitalize"> Monitoring </v-tab>
        </v-tabs>
        <v-tabs-items v-model="tab">
          <v-tab-item :key="1" value="tab-1" transition="fade-transition">
            <v-card flat>
              <plot :name="'pplot'" :update="updates.Physics"> </plot>
            </v-card>
          </v-tab-item>
          <v-tab-item :key="2" value="tab-2" transition="fade-transition">
            <v-card flat>
              <plot :name="'cplot'" :update="updates.Calibration"> </plot>
            </v-card>
          </v-tab-item>
          <v-tab-item :key="3" value="tab-3" transition="fade-transition">
            <v-card flat>
              <plot :name="'mplot'" :update="updates.TLBMonitoring"> </plot>
            </v-card>
          </v-tab-item>
        </v-tabs-items>
      </div>
    </div>
  `,
});

var app = new Vue({
  el: "#app",
  vuetify: new Vuetify(),
  data: {
    expert_mode: false,
    drawer: false, // if the tree view is closed or not
    isDrawerLocked: false, // if the tree view is in locked state
    c_configDirs: [],
    c_treeJson: {},
    c_loadedConfigName: null, // the name of the config file
    c_configLoading: false,
    c_socket: io(), // websockets
    activeNode: [],
    nodeStates: {},
    stateColors: {
      not_added: "grey",
      added: "brown",
      booted: "yellow",
      ready: "blue",
      running: "green",
      paused: "orange",
      error: "red",
      READY: "blue",
      RUN: "green",
      DOWN: "grey lighten-1",
      "IN TRANSITION": "blue-grey lighten-3",
      ERROR: "red",
      PAUSED: "orange"
    },
    log: [],
    fsmRules: null,
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
      RESUME: false
    },
    logged: false, // if the user is logged using cern account
    infoIsActive: false, // if the info box is active

    runState: "",
    runOnGoing: false,
    interlocked: false,
    username: "local_user",
    whoInterlocked: null,
    modulesError: { "1": [], "2": [] },
    resultCommand: "", // what the root action will return (success or error-> timeout)
    startRunDialog: false,
    endRunDialog: false,
    runNumber: "-",
    startTime: "",
    comment: "",
    runType: "",
    runTypes: ['Test', 'TestBeam', 'Calibration', 'Cosmics', 'Physics'],
    lostConnection: false,
    shutdownWarning: false,
    snackbar: { "open": false, "text": "", "color": "" },
    timeoutAxiosRequest: { timeout: 60000 },
    localOnly: false,
    modulesCrash: [],
    histograms_link: `${window.location.protocol}//${window.location.hostname}:8050`,
    sequencerExpanded: false,
    // sequencerState : {"seqNumber" : 3, "stepNumber" : 4,"totalStepsNumber": 10},
    sequencerState: {},
    lastSequencerState: {}
  },

  delimiters: ["[[", "]]"],
  mounted() {
    this.getInitState();
    this.getConfigDirs();
    this.getLog();
    this.initListeners();
  },
  methods: {
    stopSequencer() {
      axios.post("/stopSequencer").then((response) => {
        if (response.data.status == "error") {
          this.createSnackbar("error", response.data.data)
        }
      })
    },
    startSequence(configName, seqNumber, stepNumber) {
      // function called when pressing the "START" button in the sequencer dialog window
      this.sequencerExpanded = false;
      axios
        .post("/startSequencer", {
          configName: configName,
          startStep: stepNumber,
          seqNumber: seqNumber
        })
        .then((response) => {
          if (response.data["status"] != "success") {
            this.createSnackbar(color = "error", text = "Error : Oops, something bad happened")
          }
        })
        .catch((e) => this.createSnackbar(color = "error", text = e));

    },
    processShutdown() {
      if (this.runState === "READY") { this.shutdownRun() }
      else { this.shutdownWarning = true; }
    },
    refreshPage() { location.reload() },
    shutdownRun() {
      this.shutdownWarning = false;
      this.sendROOTCommand('SHUTDOWN')
    },
    endRun() {
      this.endRunDialog = !this.endRunDialog
      this.sendROOTCommand("STOP")
    },
    startRun() {
      this.startRunDialog = !this.startRunDialog
      this.sendROOTCommand("START");
    },
    initListeners() {
      this.c_socket.on("logChng", (line) => {
        this.log.push(line);
      });

      this.c_socket.on("runStateChng", (state) => {
        this.runState = state;
        this.runOnGoing = state != "DOWN" ? true : false;
      });

      this.c_socket.on("configChng", (configName) => {
        console.log("Personne qui a locked change de file ", configName);
        if (
          configName != this.c_loadedConfigName &&
          this.c_configLoading == false
        ) {
          this.loadConfig(configName);
        }
      });

      this.c_socket.on("refreshConfig", () => {
        this.loadConfig(this.c_loadedConfigName)
      });

      this.c_socket.on("interlockChng", (newState) => {
        this.whoInterlocked = newState;
        if (newState == undefined) {
          this.startRunDialog = false;
          this.endRunDialog = false;
          this.sequencerExpanded = false;
        }
      });

      this.c_socket.on("errorModChng", (newErrorsMod) => {
        this.modulesError = newErrorsMod;
      });

      this.c_socket.on("disconnect", () => {
        this.lostConnection = true;
      });

      this.c_socket.on("runInfoChng", (newRunInfo) => {
        this.comment = newRunInfo["runComment"]
        this.runType = newRunInfo["runType"]
        this.runNumber = newRunInfo["runNumber"]
      });

      this.c_socket.on("snackbarEmit", (msg) => {
        this.createSnackbar(color = msg.type, text = msg.message)
      })

      this.c_socket.on("crashModChng", (newCrashMod) => {
        this.modulesCrash = newCrashMod;
      })

      this.c_socket.on("sequencerStateChange", (newState) => {
        console.log(newState)
        if (Object.keys(newState) != 0) {
          console.log("Update of the sequence")
          console.log(newState)
          this.sequencerState = newState
          this.lastSequencerState = newState
        }
        else {
          console.log("End of sequence")
          console.log(newState)
          this.sequencerState = {}
        }
      })
    },
    interlock() {
      axios
        .post("/interlock", {
          action: this.locked ? "unlock" : "lock",
          cern_upn: this.username,
        })
        .then((response) => {
          console.log(response.data);
        })
        .catch((e) => console.log(e));
    },

    openFullLog() {
      window.open(`/fullLog/${this.activeNode[0].name}`, "_blank");
    },

    openLogWindow() {
      // first checks the group na`e and the machine where the logs are located (TODO)
      axios.get(`/logURL?module=${this.activeNode[0].name}`).then((r) => {
        let url = r.data;
        window.open(url, "_blank", "width=800, height=500");
      });
    },

    isROOTButtonEnabled(RCommand) {
      if (this.runState !== '' && this.fsmRules) {
        return this.runState !== '' ? this.fsmRules[this.runState].includes(RCommand) : false;
      }
      else { return false }
    },
    goToRunningConfig(runningConfig) {
      console.log("Going to ", runningConfig);
    },
    getInitState() {
      axios.get("/appState").then((response) => {
        data = response["data"];
        this.runState = data["runState"];
        this.logged = data["user"]["logged"];
        this.username = data["user"]["name"];
        this.runOnGoing = data["runOngoing"];
        this.whoInterlocked = data["whoInterlocked"];
        if (
          data["loadedConfig"] != "" &&
          this.c_loadedConfigName != data["loadedConfig"]
        ) {
          this.loadConfig(data["loadedConfig"]);
        }
        this.modulesError = data["errors"];
        this.modulesCrash = data["crashedM"]
        this.runType = data["runType"];
        this.comment = data["runComment"];
        this.runNumber = data["runNumber"];
        this.localOnly = data["localOnly"];
        this.sequencerState = data["sequencerState"]
        this.lastSequencerState = data["lastSequencerState"]
      });
    },
    getFSM() {
      axios.get("fsmrulesJson").then((response) => {
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
      this.infoIsActive = node ? true : false
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
      window.location.assign("/login");
    },
    logout() {
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
      args = { 'command': action }

      if (action === "START" || action === "STOP") {
        args["runType"] = this.runType;
        args["runComment"] = this.comment;
      }

      axios
        .post(`/processROOTCommand`, args, this.timeoutAxiosRequest)
        .then((result) => {
          this.processing[action] = false;

          if (result.data.includes("Error")) {
            this.createSnackbar("error", result.data)
          }
        })
        .catch((e) => {
          this.processing[action] = false;
          this.createSnackbar("error", e)
        });
    },

    createSnackbar(color, text) {
      this.snackbar["color"] = color
      this.snackbar["text"] = text
      this.snackbar["open"] = true
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
    isInconsistent(name) {
      if (name) {
        a = this.nodeStates[name][1]
        return a
      }
      else {
        return false
      }

    },
    hasWarning(name) {
      if (this.modulesError || this.modulesError != {} || this.modulesError != undefined) {
        return this.modulesError['1'].includes(name)
      }
      return false
    },
    hasError(name) {
      if (this.modulesError || this.modulesError != {} || this.modulesError != undefined) {
        return this.modulesError['2'].includes(name)
      }
      return false
    },
    isModuleCrashed(name) {
      if (this.modulesCrash || this.modulesCrash != {} || this.modulesCrash != undefined) {
        return this.modulesCrash.includes(name)
      }
      return false
    }
  },
  watch: {
    c_loadedConfigName: {
      handler: function (newConfig, oldConfig) {
        console.log("old", oldConfig, "new", newConfig);
        // we close the old listener for nodeStates
        this.c_socket.off("stsChng", function () {
          console.log("removed listener");
        });
        // we create the new listener for nodeStates
        this.c_socket.on("stsChng", (result) => {
          this.nodeStates = result;
        });
      },
    },
  },
  computed: {
    isSequencer: function () {
      return Object.keys(this.sequencerState).length != 0
    },
    rootNode: function () {
      // can have a different name than root
      return this.c_treeJson["name"];
    },
    locked: function () {
      if (this.username == this.whoInterlocked && this.logged) {
        return true;
      } else {
        return false;
      }
    },

    d_loadConfig: function () {
      if (this.runOnGoing || !this.locked) {
        return true;
      } else {
        return false;
      }
    },
    d_controlPanel: function () {
      return !this.locked ? true : false;
    },
    d_includeExclude: function () {
      return this.runOnGoing || !this.locked ? true : false;
    },

    isCrash: function () {
      return !(this.modulesCrash.length == 0)
    },

  },
});
