use clap::{Args, Parser, Subcommand, ValueEnum};
use std::path::PathBuf;

#[derive(Parser, Debug)]
#[command(
    name = "magi-unreal-axi",
    version,
    about = "Build, inspect, run, and safely automate the current Unreal project",
    after_help = "Examples:\n  magi-unreal-axi\n  magi-unreal-axi --project ./Game.uproject project doctor\n  magi-unreal-axi engine list"
)]
pub struct Cli {
    #[arg(long, global = true, value_name = "PATH", value_parser = nonempty_path)]
    pub project: Option<PathBuf>,
    #[arg(long, global = true, value_name = "PATH", value_parser = nonempty_path)]
    pub engine: Option<PathBuf>,
    #[arg(long, global = true, value_name = "PID", value_parser = clap::value_parser!(u32).range(1..))]
    pub editor: Option<u32>,
    #[arg(long, global = true, value_enum)]
    pub format: Option<Format>,
    #[arg(long, global = true, action = clap::ArgAction::SetTrue)]
    pub full: bool,
    #[arg(long, global = true, value_name = "SECONDS", value_parser = clap::value_parser!(u64).range(1..=86_400))]
    pub timeout: Option<u64>,
    #[arg(long, global = true, action = clap::ArgAction::SetTrue)]
    pub verbose: bool,
    #[command(subcommand)]
    pub command: Option<CommandKind>,
}

fn nonempty_path(value: &str) -> Result<PathBuf, String> {
    if value.is_empty() {
        Err("path cannot be empty".into())
    } else {
        Ok(PathBuf::from(value))
    }
}

fn nonempty_string(value: &str) -> Result<String, String> {
    if value.trim().is_empty() {
        Err("value cannot be empty".into())
    } else {
        Ok(value.to_owned())
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, ValueEnum)]
pub enum Format {
    Toon,
    Json,
}

#[derive(Subcommand, Debug)]
pub enum CommandKind {
    #[command(about = "Inspect, diagnose, or build selected Unreal project")]
    Project {
        #[command(subcommand)]
        command: Option<ProjectCommand>,
    },
    #[command(about = "Start, inspect, or stop selected Unreal editor")]
    Editor {
        #[command(subcommand)]
        command: EditorCommand,
    },
    #[command(about = "List or inspect levels")]
    Level {
        #[command(subcommand)]
        command: LevelCommand,
    },
    #[command(about = "List or inspect actors")]
    Actor {
        #[command(subcommand)]
        command: ActorCommand,
    },
    #[command(about = "View bounded operation receipts")]
    Operation {
        #[command(subcommand)]
        command: OperationCommand,
    },
    #[command(about = "List or inspect assets")]
    Asset {
        #[command(subcommand)]
        command: AssetCommand,
    },
    #[command(about = "Manage actor components")]
    Component {
        #[command(subcommand)]
        command: ComponentCommand,
    },
    #[command(about = "Inspect and control play sessions")]
    Play {
        #[command(subcommand)]
        command: PlayCommand,
    },
    #[command(about = "Inspect and compile Blueprints")]
    Blueprint {
        #[command(subcommand)]
        command: BlueprintCommand,
    },
    #[command(about = "Search, describe, or execute catalogued capabilities")]
    Capability {
        #[command(subcommand)]
        command: CapabilityCommand,
    },
    Log {
        #[command(subcommand)]
        command: LogCommand,
    },
    #[command(about = "Discover and inspect local Unreal installations")]
    Engine {
        #[command(subcommand)]
        command: Option<EngineCommand>,
    },
    #[command(about = "Explicitly install or maintain AXI integrations")]
    Setup {
        #[command(subcommand)]
        command: SetupCommand,
    },
    #[command(about = "Manage agent integrations and context")]
    Agent {
        #[command(subcommand)]
        command: AgentCommand,
    },
}

#[derive(Subcommand, Debug)]
pub enum LogCommand {
    #[command(about = "Show bounded tail of latest project process log")]
    Latest {
        #[arg(long, default_value_t = 100, value_parser = clap::value_parser!(u16).range(1..=1000))]
        lines: u16,
        #[arg(long, default_value_t = 65_536, value_parser = clap::value_parser!(u32).range(1..=1_048_576))]
        bytes: u32,
    },
    #[command(about = "Search bounded project process logs for literal text")]
    Search {
        #[arg(value_parser = nonempty_string)]
        query: String,
        #[arg(long, default_value_t = 50, value_parser = clap::value_parser!(u16).range(1..=200))]
        limit: u16,
    },
}

#[derive(Subcommand, Debug)]
pub enum CapabilityCommand {
    #[command(
        about = "Search compact generated capability metadata",
        after_help = "Examples:\n  magi-unreal-axi capability search actor\n  magi-unreal-axi capability search 'current level' --limit 10"
    )]
    Search {
        #[arg(value_parser = nonempty_string)]
        query: String,
        #[arg(long, default_value_t = 50, value_parser = clap::value_parser!(u16).range(1..=50))]
        limit: u16,
    },
    #[command(about = "Return complete generated contract for one capability")]
    Describe {
        #[arg(value_parser = nonempty_string)]
        id: String,
    },
    #[command(about = "Execute one catalogued capability through validated input")]
    Execute {
        #[arg(value_parser = nonempty_string)]
        id: String,
        #[arg(long, value_name = "JSON", conflicts_with = "input_file")]
        input_json: Option<String>,
        #[arg(long, value_name = "PATH|-", conflicts_with = "input_json")]
        input_file: Option<PathBuf>,
        #[arg(long)]
        expected_revision: Option<String>,
        #[arg(long)]
        idempotency_key: Option<String>,
    },
}

#[derive(Args, Debug)]
pub struct ListArgs {
    #[arg(long, default_value_t = 100, value_parser = clap::value_parser!(u16).range(1..=100))]
    pub limit: u16,
    #[arg(long, value_name = "CURSOR", value_parser = nonempty_string)]
    pub cursor: Option<String>,
    #[arg(long, value_delimiter = ',', num_args = 1.., value_name = "FIELD,...")]
    pub fields: Vec<String>,
}

#[derive(Subcommand, Debug)]
pub enum LevelCommand {
    #[command(about = "Read current editor level")]
    Current,
    #[command(about = "List project levels with bounded pagination")]
    List(ListArgs),
    #[command(about = "Create exact level")]
    Create(MutationLevelArgs),
    #[command(about = "Open exact level")]
    Open(MutationLevelArgs),
    #[command(about = "Explicitly save exact level")]
    Save(MutationLevelArgs),
    Settings {
        #[arg(long, value_parser = nonempty_string)]
        level_id: Option<String>,
    },
    SetGameMode {
        #[arg(long, value_parser = nonempty_string)]
        level_id: String,
        #[arg(long = "class", value_parser = nonempty_string)]
        game_mode_class: String,
        #[arg(long)]
        expected_revision: Option<String>,
    },
}

#[derive(Args, Debug)]
pub struct MutationLevelArgs {
    #[arg(long, value_parser = nonempty_string)]
    pub path: String,
    #[arg(long)]
    pub expected_revision: Option<String>,
    #[arg(long)]
    pub idempotency_key: Option<String>,
}

#[derive(Args, Debug)]
pub struct SpawnArgs {
    #[arg(long = "level", value_parser = nonempty_string)]
    pub level_id: String,
    #[arg(long = "class", value_parser = nonempty_string)]
    pub class_path: String,
    #[arg(long, value_parser = nonempty_string)]
    pub agent_key: String,
    #[arg(long)]
    pub label: Option<String>,
    #[arg(long, value_name = "X,Y,Z")]
    pub location: Option<String>,
    #[arg(long)]
    pub idempotency_key: Option<String>,
}

#[derive(Args, Debug)]
pub struct DeleteArgs {
    #[arg(value_parser = nonempty_string)]
    pub id: String,
    #[arg(long)]
    pub expected_revision: Option<String>,
    #[arg(long)]
    pub force: bool,
    #[arg(long)]
    pub dry_run: bool,
}

#[derive(Args, Debug)]
pub struct TransformArgs {
    #[arg(value_parser = nonempty_string)]
    pub id: String,
    #[arg(long, value_name = "X,Y,Z")]
    pub location: String,
    #[arg(long)]
    pub expected_revision: Option<String>,
}

#[derive(Subcommand, Debug)]
pub enum ActorCommand {
    #[command(about = "List actors in current editor level")]
    List(ListArgs),
    #[command(about = "Read actor by level package and ActorGuid")]
    View {
        #[arg(value_parser = nonempty_string)]
        id: String,
    },
    #[command(about = "Spawn actor using level-scoped agent key")]
    Spawn(SpawnArgs),
    #[command(about = "Update exact actor transform")]
    UpdateTransform(TransformArgs),
    #[command(about = "Delete exact actor")]
    Delete(DeleteArgs),
}

#[derive(Subcommand, Debug)]
pub enum OperationCommand {
    #[command(about = "View operation receipt")]
    View {
        #[arg(value_parser = nonempty_string)]
        id: String,
    },
}
#[derive(Subcommand, Debug)]
pub enum AssetCommand {
    #[command(about = "List project assets without loading them")]
    List(ListArgs),
    #[command(about = "Read asset registry metadata by object path")]
    View {
        #[arg(value_parser = nonempty_string)]
        id: String,
    },
    Save(RevisionArgs),
}

#[derive(Subcommand, Debug)]
pub enum EditorCommand {
    #[command(about = "Start selected project editor and wait for authenticated bridge")]
    Start,
    #[command(about = "Probe authenticated editor bridge")]
    Status,
    #[command(about = "Request graceful plugin-mediated editor shutdown")]
    Stop,
    #[command(about = "Describe bridge protocol and supported bootstrap operations")]
    Describe,
}

#[derive(Subcommand, Debug)]
pub enum ProjectCommand {
    #[command(
        about = "Show selected project descriptor and resolution",
        after_help = "Examples:\n  magi-unreal-axi project view\n  magi-unreal-axi --project ./Game.uproject project view"
    )]
    View,
    #[command(
        about = "Diagnose project, engine, and plugin state",
        after_help = "Examples:\n  magi-unreal-axi project doctor\n  magi-unreal-axi --project ./Game.uproject project doctor --format json"
    )]
    Doctor,
    #[command(about = "Build project editor target with UnrealBuildTool")]
    Build(ProcessArgs),
    #[command(about = "Run matching Unreal Automation tests")]
    Test(TestArgs),
    #[command(about = "Cook project content for Mac")]
    Cook(OutputProcessArgs),
    #[command(about = "Cook and package project for Mac")]
    Package(PackageArgs),
}

#[derive(Args, Debug)]
pub struct ProcessArgs {
    #[arg(
        long,
        help = "Show exact executable, arguments, cwd, and environment without execution"
    )]
    pub dry_run: bool,
}

#[derive(Args, Debug)]
pub struct TestArgs {
    #[command(subcommand)]
    pub command: TestCommand,
}

#[derive(Subcommand, Debug)]
pub enum TestCommand {
    #[command(about = "List matching Unreal Automation tests without running them")]
    List {
        #[arg(long, value_parser = nonempty_string)]
        filter: Option<String>,
        #[arg(long, default_value_t = 100, value_parser = clap::value_parser!(u16).range(1..=1000))]
        limit: u16,
        #[arg(long)]
        dry_run: bool,
    },
    #[command(about = "Run matching Unreal Automation tests")]
    Run {
        #[arg(long, value_parser = nonempty_string)]
        filter: String,
        #[arg(long, value_name = "PATH")]
        report: Option<PathBuf>,
        #[arg(long)]
        dry_run: bool,
    },
}

#[derive(Args, Debug)]
pub struct OutputProcessArgs {
    #[arg(long, value_name = "DIRECTORY", required = true)]
    pub output: PathBuf,
    #[arg(long)]
    pub dry_run: bool,
}

#[derive(Args, Debug)]
pub struct PackageArgs {
    #[arg(long, value_name = "DIRECTORY", required = true)]
    pub output: PathBuf,
    #[arg(long, help = "Allow non-empty package destination")]
    pub force: bool,
    #[arg(long)]
    pub dry_run: bool,
}

#[derive(Subcommand, Debug)]
pub enum EngineCommand {
    #[command(about = "List validated local Unreal installations")]
    List,
    #[command(about = "Show resolved Unreal installation and tools")]
    View,
}

#[derive(Subcommand, Debug)]
pub enum SetupCommand {
    #[command(about = "Install, inspect, update, repair, or uninstall bundled editor plugin")]
    Plugin {
        #[command(subcommand)]
        command: PluginCommand,
    },
    #[command(
        about = "Install agent skills and supported session integrations",
        after_help = "Examples:\n  magi-unreal-axi setup agents\n  magi-unreal-axi setup agents --claude\n  magi-unreal-axi setup agents --codex --opencode"
    )]
    Agents(AgentSetupArgs),
}

#[derive(Args, Debug)]
pub struct AgentSetupArgs {
    #[arg(long)]
    pub claude: bool,
    #[arg(long)]
    pub codex: bool,
    #[arg(long)]
    pub opencode: bool,
}

#[derive(Subcommand, Debug)]
pub enum PluginCommand {
    #[command(
        about = "Install embedded plugin payload",
        after_help = "Examples:\n  magi-unreal-axi setup plugin install --dry-run\n  magi-unreal-axi setup plugin install"
    )]
    Install(PluginMutationArgs),
    #[command(about = "Inspect plugin ownership, version, and compatibility")]
    Status,

    #[command(about = "Replace older managed payload")]
    Update(PluginMutationArgs),
    #[command(about = "Restore embedded payload and managed descriptor entry")]
    Repair(PluginMutationArgs),
    #[command(about = "Remove unchanged managed payload")]
    Uninstall(PluginMutationArgs),
}

#[derive(Subcommand, Debug)]
pub enum AgentCommand {
    #[command(
        about = "Emit compact current project context for agent sessions",
        after_help = "Examples:\n  magi-unreal-axi agent context\n  magi-unreal-axi agent context --format json"
    )]
    Context,
}

#[derive(Args, Debug)]
pub struct PluginMutationArgs {
    #[arg(long)]
    pub dry_run: bool,
    #[arg(
        long,
        help = "Preserve modified or unmanaged plugin tree as backup before replacement/removal"
    )]
    pub force: bool,
}

#[derive(Args, Debug)]
pub struct RevisionArgs {
    #[arg(value_parser = nonempty_string)]
    pub id: String,
    #[arg(long)]
    pub expected_revision: Option<String>,
}

#[derive(Subcommand, Debug)]
pub enum ComponentCommand {
    List {
        #[arg(long, value_parser = nonempty_string)]
        actor_id: String,
        #[command(flatten)]
        args: ListArgs,
    },
    View {
        #[arg(value_parser = nonempty_string)]
        id: String,
    },
    Add(ComponentAddArgs),
    Update(ComponentUpdateArgs),
    Remove(ComponentRemoveArgs),
}

#[derive(Args, Debug)]
pub struct ComponentAddArgs {
    #[arg(long, value_parser = nonempty_string)]
    pub actor_id: String,
    #[arg(long = "class", value_parser = nonempty_string)]
    pub class_path: String,
    #[arg(long, value_parser = nonempty_string)]
    pub name: String,
    #[arg(long, value_name = "X,Y,Z")]
    pub location: Option<String>,
    #[arg(long)]
    pub expected_revision: Option<String>,
}

#[derive(Args, Debug)]
pub struct ComponentUpdateArgs {
    #[arg(value_parser = nonempty_string)]
    pub id: String,
    #[arg(long, value_name = "X,Y,Z")]
    pub location: String,
    #[arg(long)]
    pub expected_revision: Option<String>,
}

#[derive(Args, Debug)]
pub struct ComponentRemoveArgs {
    #[arg(value_parser = nonempty_string)]
    pub id: String,
    #[arg(long)]
    pub expected_revision: Option<String>,
    #[arg(long)]
    pub force: bool,
    #[arg(long)]
    pub dry_run: bool,
}

#[derive(Subcommand, Debug)]
pub enum BlueprintCommand {
    View {
        #[arg(value_parser = nonempty_string)]
        id: String,
    },
    Compile(RevisionArgs),
}

#[derive(Subcommand, Debug)]
pub enum PlayCommand {
    Start,
    Status {
        #[arg(long)]
        session_id: Option<String>,
    },
    Observe {
        #[arg(long, value_parser = nonempty_string)]
        session_id: String,
    },
    Stop {
        #[arg(long, value_parser = nonempty_string)]
        session_id: String,
    },
    Input {
        #[arg(long, value_parser = nonempty_string)]
        session_id: String,
        #[arg(value_parser = nonempty_string)]
        key: String,
        #[arg(long, value_enum)]
        event: PlayEvent,
    },
    Screenshot {
        #[arg(long, value_parser = nonempty_string)]
        session_id: String,
        #[arg(long)]
        path: Option<String>,
    },
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, ValueEnum)]
pub enum PlayEvent {
    Pressed,
    Released,
}
