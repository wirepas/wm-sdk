const versions = [
'master',
'v1.1.1',
'v1.1.5',
'latest'
];

source_path = "wm-sdk";

function getRootPath() {
    let path_array = window.location.pathname.split(source_path);
    return path_array[0] + source_path + "/";
}

function getVersion() {
    return window.location.pathname.replace(getRootPath(),"").split("/")[0];
}

function handleVersionSelect(elem) {
    let version = elem.value;
    if (version == 'latest') {
        version = versions[versions.length - 2];
    }
    window.location = getRootPath() + version + "/index.html";
}
  
function populateSelector() {
    // Get current version based on path
    let cur_version = getVersion();
    let version_selector = document.getElementById("selectversion_list");
      
    if (versions.length > 1) {
        document.getElementById("selectversion_legend").style.display = "block";
        document.getElementById("selectversion_list").style.display = "block";
        versions.forEach(version => {
              let option = document.createElement("OPTION");
              version_selector.options.add(option);
              option.text = version;
              option.value = version;
              if (version == cur_version) {
                  option.selected="selected";
              }
        });
                
        // Set title correctly
        document.getElementById("projectname").innerText = "Wirepas Mesh SDK " + cur_version;        
    }
 }

 window.onload = populateSelector;
