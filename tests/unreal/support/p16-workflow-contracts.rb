#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "yaml"

module P16WorkflowContracts
  module_function

  ROOT = File.expand_path("../../..", __dir__)
  MANIFEST = JSON.parse(File.binread(File.join(ROOT, "tests/unreal/p1.6-manifest.json")))
  CLOSURE = MANIFEST.fetch("closureOracle")
  ACTIONS = CLOSURE.fetch("pinnedActions")
  WORKFLOWS = {
    "certification" => CLOSURE.dig("producerWorkflows", "certification"),
    "agentEvidence" => CLOSURE.dig("producerWorkflows", "agentEvidence"),
    "independentReview" => CLOSURE.dig("producerWorkflows", "independentReview"),
    "assembly" => CLOSURE.fetch("assemblyWorkflow"),
    "release" => CLOSURE.fetch("releaseWorkflow")
  }.freeze

  def fail!(message)
    raise "P1.6 workflow contract failed: #{message}"
  end

  def raw(name)
    File.binread(File.join(ROOT, WORKFLOWS.fetch(name)))
  end

  def action_refs(text, action)
    text.scan(%r{uses:\s*#{Regexp.escape(action)}@([^\s#]+)}).flatten
  end

  def require_text(name, *fragments)
    text = raw(name)
    fragments.each { |fragment| fail! "#{name} missing #{fragment.inspect}" unless text.include?(fragment) }
  end

  def validate
    WORKFLOWS.each_value { |path| YAML.parse_file(File.join(ROOT, path)) }
    combined = WORKFLOWS.keys.map { |name| raw(name) }.join("\n")
    {
      "actions/checkout" => ACTIONS.fetch("checkout"),
      "actions/upload-artifact" => ACTIONS.fetch("upload"),
      "actions/download-artifact" => ACTIONS.fetch("download")
    }.each do |action, expected|
      refs = action_refs(combined, action)
      fail! "missing #{action}" if refs.empty?
      fail! "#{action} pin mismatch" unless refs.all? { |ref| ref == expected }
    end
    combined.scan(/^\s*uses:\s*([^\s#]+)@([^\s#]+)/).each do |action, ref|
      next if action.start_with?("./")
      fail! "unpinned action #{action}@#{ref}" unless ref.match?(/\A[0-9a-f]{40}\z/)
    end
    require_text("certification", "runs-on: [self-hosted, macOS, arm64, unreal-5.8.1]", "environment: p16-certification", "P16_EVIDENCE_PATH_FILE=\"$evidence_path_file\"", "name: p16-release-archive", "name: p16-combined-evidence")
    require_text("agentEvidence", "environment: p16-agent-evidence", "P16_AGENT_EVIDENCE_ROOT", "materialize_verified", "name: p16-agent-run")
    require_text("independentReview", "runs-on: [self-hosted, macOS, arm64, p16-review]", "environment: p16-independent-review", "P16_INDEPENDENT_REVIEW_ROOT", "name: p16-independent-review")
    require_text("assembly", "environment: p16-release-gate", *CLOSURE.fetch("assemblyInputs").map { |input| "#{input}:" }, *CLOSURE.fetch("assemblyArtifacts").map { |artifact| "name: #{artifact}" })
    require_text("release", "environment: p16-release-gate", "dtolnay/rust-toolchain@#{ACTIONS.fetch("rustToolchain")}", "taiki-e/install-action@#{ACTIONS.fetch("installAction")}", "gh api --method POST", "-F draft=true", "-F draft=false")
    fail! "protected environments mismatch" unless CLOSURE.fetch("protectedEnvironments").sort == %w[p16-agent-evidence p16-certification p16-independent-review p16-release-gate].sort
    true
  end
end

if $PROGRAM_NAME == __FILE__
  begin
    P16WorkflowContracts.validate
    puts "P1.6 workflow contracts: PASS"
  rescue StandardError => error
    warn error.message
    exit 1
  end
end
